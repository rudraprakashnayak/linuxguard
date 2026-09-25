# LinuxGuard Requirements

## Functional Requirements

### FR1: Process Management
- Start configured processes using fork/exec
- Stop processes gracefully (SIGTERM → SIGKILL)
- Track process state (STOPPED, STARTING, RUNNING, STOPPING, FAILED, BACKOFF)
- Track PID for each managed process

### FR2: Automatic Restart
- Detect process failures via waitpid(WNOHANG)
- Automatically restart failed processes
- Configurable maximum restart count
- Exponential backoff between restart attempts
- Configurable maximum backoff duration

### FR3: Dependency Management
- Define service dependencies in configuration
- Start services in topological order
- Stop services in reverse topological order
- Detect and reject circular dependencies
- Skip service start if dependencies are not running

### FR4: Resource Monitoring
- Monitor CPU usage via /proc
- Monitor RAM usage (RSS) via /proc
- Monitor virtual memory usage
- Monitor thread count
- Configurable CPU and RAM limits
- Log warnings when limits exceeded

### FR5: Configuration
- Human-readable INI-style configuration format
- Global settings (socket path, PID file, log file, log level, monitor interval)
- Per-service settings (command, working directory, user, restart policy, resource limits, dependencies)
- Support comments with #

### FR6: CLI Control
- status <name>: Show detailed status of a service
- list: List all services and their states
- start <name>: Start a service
- stop <name>: Stop a service
- restart <name>: Restart a service
- reload: Reload configuration (placeholder)
- shutdown: Stop the daemon

### FR7: Logging
- Thread-safe logging
- Timestamped log entries
- Configurable log levels (DEBUG, INFO, WARN, ERROR)
- File or stderr output

### FR8: IPC
- Unix domain socket communication
- Command-response protocol
- Thread-safe server

### FR9: Daemon Mode
- Run as background service
- Graceful shutdown on SIGTERM/SIGINT
- PID file management
- systemd integration

## Non-Functional Requirements

### NFR1: Performance
- Minimal CPU overhead when idle
- Monitor interval configurable (default 5 seconds)
- Non-blocking process status checks

### NFR2: Reliability
- No external dependencies (only Linux/POSIX APIs and STL)
- Proper error handling throughout
- Thread-safe shared state
- Graceful degradation on failures

### NFR3: Portability
- C++17 standard
- Linux/POSIX only
- CMake build system

### NFR4: Security
- Optional user switching for services
- systemd security hardening options
- No unnecessary privileges

### NFR5: Maintainability
- Clean modular architecture
- Comprehensive unit tests
- Clear documentation
- Interview-defensible code

## Technical Constraints

- C++17 only
- Linux/POSIX APIs only
- CMake build system
- systemd integration
- Unix domain sockets for IPC
- STL threads/mutex where needed
- Minimal/no external dependencies
- Compile with -Wall -Wextra -Wpedantic

## Test Requirements

- Unit tests for ProcessManager
- Unit tests for ConfigParser
- Unit tests for DependencyGraph
- Integration tests for daemon lifecycle
- End-to-end validation:
  - Start daemon
  - Start managed process
  - Verify status
  - Kill process externally
  - Verify automatic restart
  - Test CLI IPC commands
  - Test graceful shutdown
