# Git & GitHub Beginner's Guide

Welcome to GitHub! Think of Git as a **Time Machine** for your code. It lets you save "checkpoints" that you can return to if things break.

### Key Terms (The "Lingo")

| Term | Analogy | Description |
| :--- | :--- | :--- |
| **Repository (Repo)** | The Project Folder | The container for all your code files and their history. |
| **Commit** | A Save Point | A snapshot of your code at a specific moment. Each commit has a "message" (your **patch notes**). |
| **Push** | Uploading | Sending your local "save points" (commits) from your PC to the internet (GitHub). |
| **Pull** | Downloading | Getting the latest "save points" from the internet to your PC. |
| **Fork** | A Copy | A personal copy of someone else's project that you can change without affecting the original. |
| **Remote (origin/upstream)** | The Address Book | Links to where the code lives on GitHub. `origin` is YOUR copy; `upstream` is the original. |
| **Branch** | A Parallel Timeline | A way to work on new features without breaking the main version of the code. |

### Your Core Workflow

When you're ready to save and upload your progress:

1. **`git add .`** - Prepare your changes (the "Stage").
2. **`git commit -m "Your notes"`** - Save your changes locally (the "Commit").
3. **`git push origin master`** - Upload your changes to GitHub (the "Push").

### Best Practices
- **Commit Often**: Smaller commits make it easier to find bugs.
- **Good Messages**: Instead of "Update", use "Fixed ESP rendering and added custom colors".
- **Check Status**: Run `git status` often to see what files are changed.
