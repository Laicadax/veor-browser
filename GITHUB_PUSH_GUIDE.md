# VEOR Browser — GitHub Push Guide

## Step 1: Create Repository on GitHub

1. Go to https://github.com/new
2. Repository name: `veor-browser`
3. Description: `VEOR Browser — A precision-engineered browser built on Chromium Content API`
4. Visibility: Public or Private (your choice)
5. **DO NOT** initialize with README, .gitignore, or license (we already have them)
6. Click "Create repository"

## Step 2: Push Local Repository

After creating the empty repo, GitHub will show you push instructions. Use these commands:

```bash
# If you downloaded the source tarball
tar xzf veor-browser-source.tar.gz
cd veor

# Initialize git (if not already done)
git init
git add -A
git commit -m "VEOR Browser - Initial commit"

# Add remote and push
git remote add origin https://github.com/YOUR_USERNAME/veor-browser.git
git branch -M main
git push -u origin main
```

## Step 3: Verify

Visit `https://github.com/YOUR_USERNAME/veor-browser` — you should see all 222 files.

## Alternative: Push from Bare Repo

If you have the git bundle:

```bash
tar xzf veor-browser-git.tar.gz
cd veor.git
git remote add origin https://github.com/YOUR_USERNAME/veor-browser.git
git push origin master:main
```

## Branch Protection (Recommended)

After first push, enable branch protection:
1. Settings -> Branches -> Add rule
2. Branch name pattern: `main`
3. Check: "Require a pull request before merging"
4. Check: "Require status checks to pass"

---
