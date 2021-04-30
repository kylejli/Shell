# Simple Shell - ECS 150
Professor: Samuel King

### wish: "wisconsin" shell
- runs commands
- batch mode and interactive mode
### batch mode runs commands in a given input file
- Usage: ./wish batch.txt
### interactive mode keeps asking user for command
- Usage: ./wish
### Built-in Commands
- exit
  - exits shell
- cd
  - calls chdir() to change directory
- path
  - changes which path(s) to look for commands
  - default: /bin
### Redirection
- output to file instead of STDOUT
- Usage: command > file.txt
### Parallel Commands
- run commands in parallel
- Usage: command1 & command2 & command3
