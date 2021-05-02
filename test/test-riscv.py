from runner import run_cmd, run_cmd_willsucc
import pathlib
from tqdm import tqdm

#run_cmd_willsucc('make clean')
#print('making compiler')
#out = run_cmd_willsucc('make', 120)
#print('MAKE OUTPUT: <<%s>>'%out)

out_path = pathlib.Path('test/docker_share/out.S')

for p in tqdm(sorted(list(pathlib.Path('.').glob('testcases/**/func*/*.sy')))):
    if p.name in [
        '92_matrix_add.sy', '93_matrix_sub.sy', '94_matrix_mul.sy', '95_matrix_tran.sy', '96_many_param_call.sy', '97_many_global_var.sy'
    ]:  # violate 8-arg limit
        continue

    if out_path.exists():
        out_path.unlink()

    print('trying', p)
    out = run_cmd_willsucc(f'ASAN_OPTIONS=detect_leaks=0 build/compiler -S {p} -o {out_path}')
    if out.strip():
        print('COMPILER OUTPUT: <<%s>>'%out)

    basename = p.name.rpartition('.')[0]

    with open(p.with_name(basename+'.out')) as f:
        stdres = f.read().strip()
    if p.with_name(basename+'.in').exists():
        with open(p.with_name(basename+'.in')) as f:
            inp = f.read()
    else:
        inp = ''

    with out_path.with_name('input.txt').open('w') as f:
        f.write(inp)
    
    # sudo docker run -v /home/xmcp/atoz/test/docker_share:/tmp/hostshare --rm riscv-dev-env /tmp/hostshare/judge.sh
    errno, out, stderr = run_cmd(
        f'sudo docker run'
        f' -v {out_path.parent.resolve()}:/tmp/hostshare'
        f' --rm'
        f' riscv-dev-env'
        f' /tmp/hostshare/judge.sh'
    , 10, '')
    out = (out.strip()+'\n'+str(errno)).lstrip()

    if out.strip()!=stdres.strip():
        print('OUTPUT ERROR', p)
        print('OUT: {\n%s\n}'%out)
        print('STD: {\n%s\n}'%stdres)
        print('ERR: {\n%s\n}'%stderr)
        1/0
    
    out_path.unlink()
        

print('ALL DONE!')