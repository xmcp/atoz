import pathlib
import time
from runner import run_cmd, run_cmd_willsucc

basename = pathlib.Path('testcases/sysy/section2/performance_test/fft0.sy')

out_path = pathlib.Path('test/docker_share/out.S')

out = run_cmd_willsucc(f'ASAN_OPTIONS=detect_leaks=0 build/compiler -S {basename} -o {out_path}')
print('ASM COMPILER OUT:', out)

if basename.with_suffix('.in').exists():
    with open('test/docker_share/input.txt', 'w') as f:
        f.write(basename.with_suffix('.in').open().read())

t1 = time.time()

errno, out, stderr = run_cmd(
    f'sudo docker run'
    f' -v {out_path.parent.resolve()}:/tmp/hostshare'
    f' --rm'
    f' riscv-dev-env'
    f' /tmp/hostshare/judge.sh'
    f' --timeit'
, 60, '')

t2 = time.time()

print('ASM OUT: ===')
out = (out.strip()+'\n== errno: '+str(errno)).lstrip()
print(out)
print('TIME: %.1f'%(t2-t1))