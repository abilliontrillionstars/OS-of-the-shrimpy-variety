#!/usr/bin/env python

import asyncio
import sys
import asyncio.subprocess
import time
import bmputils
import tempfile
import os

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
        P.stdin.write(b"`cquit\n")
        await asyncio.wait_for(P.wait(), 1)
        if P.returncode == None:
            P.terminate()
            await asyncio.wait_for(P.wait(), 1)
            if P.returncode == None:
                P.kill()
                await asyncio.wait_for(P.wait())

async def qprompt():
    lookingFor = "\n(qemu)"
    tmp = [""] * len(lookingFor)
    while True:
        c = await P.stdout.read(1)
        c=c.decode(errors="replace")
        tmp.pop(0)
        tmp.append(c)
        if "".join(tmp) == lookingFor:
            return



async def screencap(filename):
    tf = tempfile.NamedTemporaryFile(delete=False)
    tf.close()
    NAME = tf.name.replace("/","//")

    # ~ NAME = "debug"
    try:
        P.stdin.write(b"`c")
        await qprompt()
        P.stdin.write(f"screendump {NAME}\n".encode())
        await qprompt()
        P.stdin.write(b"`c")
        w,h,pix = bmputils.loadppm(tf.name)
        bmputils.savebmp(filename,w,h,pix,True)
    finally:
        os.unlink(tf.name)

async def main():
    global P

    bmputils.unzip("expected.bmp.zip")

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
        if line == "DONE":
            print("Got 'DONE' message")
            await screencap("actual.bmp")
            await terminate()
            ok = bmputils.compare("expected.bmp","actual.bmp","diff.bmp")
            if ok:
                print("\n\nOK!!!\n\n")
                sys.exit(0)
            else:
                print("Mismatch between expected.bmp and actual.bmp; See diff.bmp for differences")
                sys.exit(1)



asyncio.run(main())
