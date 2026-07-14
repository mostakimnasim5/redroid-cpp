# Contributing to RedroidCPP

Thank you for your interest in contributing to RedroidCPP!

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Submitting Changes](#submitting-changes)
- [Coding Standards](#coding-standards)

## 📜 Code of Conduct

This project adheres to a code of conduct that all contributors are expected to follow. Please be respectful and constructive in all interactions.

## 🚀 Getting Started

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/redroid-cpp.git
   cd redroid-cpp
   ```
3. Add the upstream remote:
   ```bash
   git remote add upstream https://github.com/mostakimnasim5/redroid-cpp.git
   ```

## 💻 Development Setup

### Prerequisites

- C++17 compatible compiler (g++ or clang++)
- CMake 3.16+
- Docker (for containerized development)
- Linux with KVM support (for emulator)

### Build from Source

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
ctest
```

### Docker Development

```bash
# Build Docker image
./tools/manage.sh build

# Start emulator
./tools/manage.sh start -m Samsung -d "Galaxy S24 Ultra"
```

## 🔧 Making Changes

1. Create a branch for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bug-fix
   ```

2. Make your changes following the coding standards below

3. Test your changes:
   ```bash
   # Build
   mkdir -p build && cd build && cmake .. && make

   # Run CLI
   ./redroid-cli help
   ```

4. Commit your changes with a clear message:
   ```bash
   git add .
   git commit -m "feat: add new feature description"
   ```

## 📤 Submitting Changes

1. Push your branch to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

2. Open a Pull Request against the `main` branch

3. Fill out the PR template with:
   - Description of changes
   - Related issue number (if applicable)
   - Testing performed
   - Screenshots (if UI changes)

## 📝 Coding Standards

### C++ Code

- Follow C++17 standard
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions small and focused
- Use namespaces to organize code

### Shell Scripts

- Use `#!/bin/bash`
- Enable strict mode: `set -euo pipefail`
- Use meaningful function names
- Add error handling
- Include usage documentation

### File Organization

```
redroid-cpp/
├── src/              # Source files
├── include/          # Header files
├── docker/           # Docker configuration
├── tools/            # Shell scripts
├── profiles/         # Device profiles
├── tests/            # Test files
└── docs/             # Documentation
```

### Git Commit Messages

- Use imperative mood: "Add feature" not "Added feature"
- Keep subject line under 72 characters
- Reference issues when applicable

Format:
```
<type>(<scope>): <subject>

<body>

<footer>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance

## 🐛 Reporting Issues

When reporting issues, please include:

- Clear description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Environment details (OS, Docker version, etc.)
- Relevant logs or screenshots

## 💡 Suggesting Features

Open an issue with the label `enhancement` and include:
- Clear use case
- Proposed solution (if any)
- Alternative solutions considered

## 📄 License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.

---

Thank you for contributing to RedroidCPP! 🎉
