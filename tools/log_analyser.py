#!/usr/bin/python3

import re
import sys
from datetime import datetime, timedelta
from typing import Dict, Final, List, Callable, Optional, Tuple, TextIO


###############################################################################

class LogEntry:
    when: datetime
    text: str
    line: int

    def __init__(self, text: str, line: int):
        self.when = datetime.strptime(text[:23], "%Y/%m/%d %H:%M:%S.%f")
        self.text = text[23:]
        self.line = line


class ParsedLog(Dict[str, List[LogEntry]]):
    def __init__(self):
        self = {}

    def merge(self, other: 'ParsedLog'):
        for pattern, logs in other.items():
            self.setdefault(pattern, []).extend(logs)

    def sort(self):
        for pattern in self.keys():
            self[pattern].sort(key=lambda entry: entry.when)

Patterns = List[Tuple[str,str]]

class LogFile:
    file: TextIO
    line_num: int = 0
    next_line: str = ''
    log_line: str = ''

    def readline(self) -> str:
        line = self.file.readline()
        if line:
            self.line_num += 1
            if self.line_num % 10000 == 0:
                print(f"Processed {self.line_num} lines...", end='\r')
        return line

    def readlog(self) -> bool:
        self.log_line = self.next_line
        self.next_line = self.readline()
        if not self.next_line:
            last_line = self.log_line
            self.log_line = ''
            return last_line

        while self.next_line and not re.search(r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}\.\d{3}', self.next_line):
            self.log_line += self.next_line
            self.next_line = self.readline()
        return True

    def parse(self, filename: str, patterns: Patterns) -> ParsedLog:
        self.file = open(filename, 'r', encoding='UTF-8', errors='ignore')
        self.line_num: int = 0
        self.next_line: str = ''
        self.log_line: str = ''

        entries = ParsedLog()
        for name, regex in patterns:
            entries[name] = []
        while self.readlog():
            for name, regex in patterns:
                if re.search(regex, self.log_line):
                    try:
                        entries[name].append(LogEntry(self.log_line.strip(), self.line_num))
                    except:
                        print(f"Could not parse line {self.line_num}\n{sys.exception()}\n{self.log_line}\n")
                    break

        print(f"Processed {self.line_num} lines in {filename}")
        return entries


def parse(files: List[str], patterns: Patterns) ->ParsedLog:
    log_file = LogFile()
    entries = log_file.parse(files[0], patterns)

    for file in files[1:]:
        entries.merge(log_file.parse(file, patterns))

    entries.sort()
    return entries

###############################################################################

class ProfileInfo:
    when: datetime
    cpu: float
    memory: float

    def __init__(self, when: datetime, cpu: float, memory: str):
        self.when = when
        self.cpu = cpu
        if memory.endswith('MB'):
            self.memory = float(memory[:-2])
        elif memory.endswith('GB'):
            self.memory = float(memory[:-2]) * 1000

    def __str__(self) -> str:
        return f"{self.when}{self.cpu:6}%{self.memory:9.1f}MB"

def show_profile(args: List[str], files: List[str], patterns: Patterns):
    logs = parse(files, patterns)

    profiles: List[ProfileInfo] = []
    for entry in logs['profile']:
        line = entry.text.splitlines()[0]
        profiles.append(
            ProfileInfo(
                entry.when,
                float(re.sub('.*CPU: ([0-9.]+).*', '\\1', line)),
                re.sub('.*Memory:.*res=(.*)', '\\1', line)
            )
        )

    max_cpu = profiles[0]
    max_mem = profiles[0]
    for profile in profiles:
        if max_cpu.cpu < profile.cpu:
            max_cpu = profile
        if max_mem.memory < profile.memory:
            max_mem = profile
        print(profile);

    print(f"\nMax CPU:\n{max_cpu}\nMax Mem:\n{max_mem}")

###############################################################################

class MutexEvent:
    event: str
    thread: str
    locker: str

    def __init__(self, entry: LogEntry, event: str, locker: str):
        self.event = event
        self.thread = entry.text[1:entry.text.index('\t', 1)]
        self.locker = locker

class MutexInfo:
    name: str
    events: Dict[datetime, MutexEvent]

    def __init__(self, name: str):
        self.name = name
        self.events = {}

    def __str__(self) -> str:
        s = f"{self.name}"
        for when, event in sorted(self.events.items()):
            s += f"\n  {when}  {event.event:<8}{event.thread}"
            if event.locker:
                s += f"  {event.locker}"
        return s

class Mutexes(Dict[str, MutexInfo]):
    def __init__(self):
        self = {}

    def add(self, entry: LogEntry, event: str, locker = ''):
        pos = entry.text.index('mutex')+6
        addr = entry.text[pos:pos+16]
        if addr not in self:
            self[addr] = MutexInfo(entry.text[entry.text.index('(', pos)+1:entry.text.index(')', pos)+1])
        self[addr].events[entry.when] = MutexEvent(entry, event, locker)

def analyse_deadlock(args: List[str], files: List[str], patterns: Patterns):
    logs = parse(files, patterns)

    mutexes = Mutexes()

    for entry in logs['possible']:
        locker = 'N/A'
        for line in entry.text.splitlines():
            if 'LOCKER' in line:
                locker = re.sub('.*thread-id=([0-9]+).*', '\\1', line)
                break;
            if locker == 'Missing':
                locker = re.sub('.*id=([0-9]+).*', '\\1', line)
                break;
            if 'Owner Thread' in line:
                locker = 'Missing'
        mutexes.add(entry, 'try-lock', locker)

    for entry in logs['release']:
        mutexes.add(entry, 'unlock')
    for entry in logs['phantom']:
        mutexes.add(entry, 'locked')

    print('\nDeadlocks:')
    for addr, mutex in mutexes.items():
        print(f"{addr}: {mutex}")


###############################################################################

def analyse_leak(args: List[str], files: List[str], patterns: Patterns):
    logs = parse(files, [ (pattern[0], args[0] + pattern[1]) for pattern in patterns])

    print(f"{len(logs['create'])} {args[0]} objects created")
    print(f"{len(logs['destroy'])} {args[0]} objects destroyed")


###############################################################################

def extract_object(args: List[str], files: List[str], patterns: Patterns):
    print('Extracting logs for object', args[0])
    patterns = [('object', args[0])]
    logs = parse(files, patterns)

    for entry in logs['object']:
        print(entry.when,entry.text)


###############################################################################
# Main execution - execute the require analysis

commands = {
    "profile": (
        show_profile,
        0,
        [
            ('profile', 'PTLibProfile')
        ],
        "Show PTLib CPU/Memory profile"
    ),
    "deadlock": (
        analyse_deadlock,
        0,
        [
            ('possible', 'Possible deadlock'),
            ('release', 'Released phantom deadlock'),
            ('phantom', 'Phantom deadlock')
        ],
        "Analyse deadlocks"
    ),
    "leak": (
        analyse_leak,
        1,
        [
            ('create', '.*Created'),
            ('destroy', '.*Destroyed'),
        ],
        "Analyse the specified object for leaking"
    ),
    "object": (
        extract_object,
        1,
        [
            ('object', ''),
        ],
        "Extract all logs associated with the object ID"
    )
};

if len(sys.argv) > 2 and sys.argv[1] in commands:
    command, arg_count, patterns, _ = commands[sys.argv[1]]
    if len(sys.argv) > 2+arg_count:
        command(sys.argv[2:2+arg_count], sys.argv[2+arg_count:], patterns)
        sys.exit(0)

print("Usage: log_analyser.py <cmd> [ option ... ] <log_file> [ <log_file> ... ]\n" +
     "\n".join([f"  {cmd}: {info[3]}" for cmd, info in commands.items()]))
sys.exit(1)
