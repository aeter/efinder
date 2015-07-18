### findster

A fast text searching tool with focus on simplicity.<br />
Usage: ```./fe <regex> <dir>```

- Features
  - [x] searching with regex patterns
  - [x] search in pwd when no dir is specified
  - [x] handle colour if stdout is piped/tty
  - [ ] search stdin when piped
  - [ ] ignore searching binary files
  - [ ] ignore searching ".git" dirs (annoying)

- Known bugs
  - [ ] fix the case of "./fe main .. > out" which loops writing big file in pwd
  - [ ] add tests
