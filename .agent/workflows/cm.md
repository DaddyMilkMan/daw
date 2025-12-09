---
description: Commit and merge changes to master (shortcut 'cm')
---
1. Ensure you are on the branch you want to merge (e.g., `git checkout <branch>`).
2. Pull latest changes from remote master:
   ```
   git checkout master
   git pull origin master
   ```
3. Switch back to your working branch (if not already on master):
   ```
   git checkout <your-branch>
   ```
4. Add all changes:
   ```
   git add .
   ```
5. Commit with a message (you can provide a custom message or use a default one):
   ```
   git commit -m "User commit"
   ```
6. Merge your branch into master:
   ```
   git checkout master
   git merge <your-branch>
   ```
7. Push the updated master to the remote repository:
   ```
   git push origin master
   ```
**Note:** Replace `<your-branch>` with the name of your current feature branch. If you are already on `master`, steps 2, 3, and 6 can be skipped, and you can directly add, commit, and push.
