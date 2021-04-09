from runner import run_cmd, run_cmd_willsucc

out = run_cmd_willsucc(f'build/compiler -S -e test/src.c -o test/out.S')
print('COMPILER OUT:', out)

errno, out, stderr = run_cmd(f'test/MiniVM/build/minivm test/out.S', 10, '')
print('=====')
out = (out.strip()+'\n== errno: '+str(errno)).lstrip()
print(out)