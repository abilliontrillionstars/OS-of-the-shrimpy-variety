#!/usr/bin/env python

import asyncio
import sys
import asyncio.subprocess
import time
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


async def testIt(expected,TEST):
    global P

    P = await asyncio.create_subprocess_exec(
                sys.executable, "make.py",
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                limit=65536
    )
    try:
        data = ""
        deadline = time.time() + TIMEOUT

        while True:

            timeLeft = deadline - time.time()
            try:
                c = await asyncio.wait_for(P.stdout.read(1), timeLeft)
            except TimeoutError:
                return False,"Time exceeded"

            c = c.decode(errors="ignore")
            sys.stdout.write(c)
            sys.stdout.flush()
            data = data + c
            data = data[-500:]

            found = [
                0,
                1 if expected[1] in data else 0,
                1 if expected[2] in data else 0,
                1 if expected[3] in data else 0
            ]

            total = sum(found)
            if total == 0:
                #none of them found; try again
                continue

            if total == 1:
                #exactly one found; make sure it's the right one
                if found[TEST]:
                    return True,f"{expected[TEST]} OK!"
                else:
                    tmp = f"Expected {expected[TEST]} but got "
                    for i in range(4):
                        if found[i]:
                            tmp += expected[i]+" "
                    return False,tmp

            if total >  1:
                tmp = f"Expected {expected[TEST]} but got "
                for i in range(4):
                    if found[i]:
                        tmp += expected[i]+" "
                return False,tmp
    finally:
        await terminate()



async def main():
    global P


    expected=[
        "xx",
        "Divide by zero",
        "Undefined opcode",
        "General fault"
    ]

    with open("testsuite.c","r") as fp:
        testsuite = fp.read()

    try:
        for TEST in [1,2,3]:

            print("\n\nTesting",expected[TEST],"...\n\n")

            idx = testsuite.find("///MAGIC GOES HERE")
            assert idx != -1

            data = testsuite[:idx]+f"TEST={TEST};"+testsuite[idx:]

            with open("testsuite.c","w") as fp:
                fp.write(data)

            ok,message = await testIt(expected,TEST)
            if not ok:
                print("\n\n")
                print(message)
                return

        print("\n\nAll OK!")

    finally:
        with open("testsuite.c","w") as fp:
            fp.write(testsuite)

asyncio.run(main())
