# nju-ics-exp

It's the repo recording the process of @taoky, @iBug & @breakingdevil doing [the ICS experiments of NJU](https://legacy.gitbook.com/book/nju-ics/ics2018-programming-assignment).

Let's begin.

## Configure the environment

Do as PA0, but do not clone the official repo. Instead, fork this repo to your account, and clone from your own repo to your home folder.

DO NOT RUN `init.sh`, instead:

1. Rename your project folder to `ics2018`

2. Add these lines to `~/.bashrc`

```
export NEMU_HOME=/home/ics/ics2018/nemu
export AM_HOME=/home/ics/ics2018/nexus-am
export NAVY_HOME=/home/ics/ics2018/navy-apps
```

3. Run `source ~/.bashrc`

4. Modify `nemu/Makefile.git`. Change `STUID` to any value you like (e.g. `233333333`), and `STUNAME` to your nickname.

Please note that when you run `make` in nemu folder, a new commit message will be created.

If you find that you can't push to your own repo, configure "deploy key". (RTFM)

## To-do

@taoky is working on PA0, and will configure this repo.
