# Local Generals.io v6 (Qt) Commit Regulations

## Introduction

This document outlines the commit regulations for the development of Local Generals.io v6 (Qt). The goal is to ensure consistency, clarity, and maintainability in the project’s commit history. By following these guidelines, we can improve collaboration and make it easier for contributors to understand the progress of the project.

## Commit Message Guidelines

### 1. **Format**

Each commit message should follow this structure:

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

- **type**: A noun describing the purpose of the commit (e.g., `feat`, `fix`, `docs`, `style`, `refactor`, `chore`).
- **scope**: The area of the codebase the commit affects (e.g., `ui`, `core`, `networking`). The scope is optional and should only be used when relevant.
- **subject**: A short summary of what the commit does. Capitalize the first letter, and keep it under 72 characters.
- **body**: A detailed description of the changes made. Explain why the changes were necessary and how they were implemented. Keep the body wrapped at 72 characters per line.
- **footer**: Any relevant issue numbers or breaking changes. For example, `Closes #42`, `BREAKING CHANGE: Change API`, etc.

### 2. **Types of Commit Messages**

#### - `feat`: A new feature.

Example:
```
feat(ui): add custom skin selection option
```

#### - `fix`: A bug fix.

Example:
```
fix(networking): resolve connection issue during login
```

#### - `docs`: Documentation changes.

Example:
```
docs(readme): update installation instructions
```

#### - `style`: Code style changes (e.g., formatting, linting).

Example:
```
style(ui): improve button alignment
```

#### - `refactor`: Refactoring code without changing functionality.

Example:
```
refactor(core): simplify game state management logic
```

#### - `chore`: Changes to the build process, dependencies, or maintenance tasks.

Example:
```
chore(deps): update Qt framework to v6.3
```

#### - `test`: Adding or updating tests.

Example:
```
test(ui): add tests for player selection screen
```

#### - `ci`: Continuous Integration related changes.

Example:
```
ci: add deployment pipeline for production
```

### 3. **Commit Subjects**

- Always start with a verb in the imperative form (e.g., `add`, `remove`, `fix`, `update`).
- Be concise but descriptive. Avoid vague commit messages like "Fix stuff" or "Changes made."
- Use lowercase letters unless the commit message is a proper noun or acronym.

### 4. **Example Commit Messages**

- **Correct Example:**
  
  ```
  feat(core): add multiplayer mode support
  ```

- **Incorrect Example:**
  ```
  Adding multiplayer mode
  ```

## Commit Size and Frequency

### 1. **Commit Size**

- **Small and Focused**: Each commit should focus on a single logical change. Keep the size manageable so that it’s easy to review and understand. 
- **Atomic Commits**: Each commit should represent a self-contained change that can be tested or reverted independently.
- **Avoid Large, Unfocused Commits**: Avoid making commits that change multiple unrelated areas of the project at once, as this makes it harder to understand the impact of the change.

### 2. **Commit Frequency**

- **Commit Frequently**: Commit your changes often to keep the project history up to date and to make the changes easier to track.
- **Don’t Wait for Large Changes**: Don’t wait until the end of a large feature or bug fix to commit. Break down your work into smaller parts and commit each one. This also allows others to see progress in real time.
- **Avoid Too Many Small Commits**: While frequent commits are encouraged, avoid making tiny, insignificant commits (e.g., "fix typo" or "change formatting"). This can clutter the commit history and slow down collaboration.

### 3. **Commit Practices**

- **Write Descriptive Commit Messages**: As mentioned in the guidelines, always write meaningful and descriptive commit messages so others understand the changes you’ve made.
- **Rebase Before Merging**: Before merging your feature branch into `develop` or `main`, rebase it to ensure a clean and linear history.
- **Squash Commits**: If you have several small commits that can be logically grouped together, squash them before merging into the main branches. This reduces unnecessary noise in the commit history.
