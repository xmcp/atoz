import pathlib
from runner import run_cmd, run_cmd_willsucc

out_path = pathlib.Path('test/docker_share/out.S')

out = run_cmd_willsucc(f'build/compiler -S -t test/src.c -o {out_path}')
print('TIGGER COMPILER OUT:', out)

errno, out, stderr = run_cmd(f'test/MiniVM/build/minivm -t {out_path}', 10, '')
print('TIGGER OUT: ===')
out = (out.strip()+'\n== errno: '+str(errno)).lstrip()
print(out)

out = run_cmd_willsucc(f'build/compiler -S test/src.c -o {out_path}')
print('ASM COMPILER OUT:', out)

errno, out, stderr = run_cmd(
    f'sudo docker run'
    f' -v {out_path.parent.resolve()}:/tmp/hostshare'
    f' --rm'
    f' riscv-dev-env'
    f' /tmp/hostshare/judge.sh'
, 10, '')
print('ASM OUT: ===')
out = (out.strip()+'\n== errno: '+str(errno)).lstrip()
print(out)