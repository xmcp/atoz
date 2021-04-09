import subprocess

FAILFAST = False

def run_cmd(cmdline, timeout_sec, inp): # errno, stdout, stderr
    p = subprocess.Popen(
        cmdline,
        shell=True,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    try:
        pout, perr = p.communicate(inp.encode(), timeout=timeout_sec)
        errcode = p.wait()
    except subprocess.TimeoutExpired:
        p.kill()
        run_cmd('killall minivm', None, '') # sometimes minivm fails to stop
        pout, perr = p.communicate(timeout=timeout_sec)
        errcode = -1

    return errcode, pout.decode('utf-8', 'replace'), perr.decode('utf-8', 'replace')

def run_cmd_willsucc(cmdline, timeout_sec=10, stdin=''):
    errno, stdout, stderr = run_cmd(cmdline, timeout_sec, stdin)

    if errno:
        if FAILFAST:
            print('EXEC FAILED', errno, cmdline)
            print('STDERR: {\n%s\n}'%stderr)
            print('STDOUT: {\n%s\n}'%stdout)
            1/0
        return f'{stdout}\n===\n{stderr}\n===\n(errno is {errno})'

    return stdout