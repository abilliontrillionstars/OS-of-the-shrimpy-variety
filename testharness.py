#!/usr/bin/env python

import asyncio
import sys
import asyncio.subprocess
import time


TIMEOUT = 30

P=None

async def getLine(timeLeft):
    line = ""

    while True:

        try:
            c = await asyncio.wait_for(P.stdout.read(1), timeLeft)
        except TimeoutError:
            if len(line) == 0:
                return None
            else:
                return line

        if len(c) == 0:
            return line

        c = c.decode(errors="replace")
        sys.stdout.write(c)
        sys.stdout.flush()

        line += c

        if c == "\n":
            return line


async def terminate():
    if P and P.returncode == None:
        P.stdin.write(b"\n\n`cquit\n")
        await asyncio.wait_for(P.wait(), 1)
        if P.returncode == None:
            P.terminate()
            await asyncio.wait_for(P.wait(), 1)
            if P.returncode == None:
                P.kill()
                await asyncio.wait_for(P.wait())


async def main():
    global P

    deadline = time.time() + TIMEOUT
    timeLeft = TIMEOUT

    P = await asyncio.create_subprocess_exec(
        sys.executable, "make.py",
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        limit=65536
    )

    while True:
        line = await getLine(timeLeft)
        if line == None:
            await terminate()
            print("\n\nTime exceeded")
            sys.exit(1)

        if len(line) == 0:
            print("Program exited?")
            await terminate()
            sys.exit(1)

        timeLeft = deadline - time.time()

        if timeLeft <= 0:
            await terminate()
            print("\n\nTime exceeded")
            sys.exit(1)

        line = line.strip()
        if line == "We the People of the United States":
            await terminate()
            print("\n\nOK!!!\n\n")
            sys.exit(0)


asyncio.run(main())
