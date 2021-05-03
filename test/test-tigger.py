from runner import run_cmd, run_cmd_willsucc
import pathlib
from tqdm import tqdm

#run_cmd_willsucc('make clean')
#print('making compiler')
#out = run_cmd_willsucc('make', 120)
#print('MAKE OUTPUT: <<%s>>'%out)

out_path = pathlib.Path('test/out.S')

for p in tqdm(sorted(list(pathlib.Path('.').glob('testcases/**/func*/*.sy')))):
    if p.name in [
        '92_matrix_add.sy', '93_matrix_sub.sy', '94_matrix_mul.sy', '95_matrix_tran.sy', '96_many_param_call.sy', '97_many_global_var.sy'
    ]:  # violate 8-arg limit
        continue

    if out_path.exists():
        out_path.unlink()

    print('trying', p)
    out = run_cmd_willsucc(f'ASAN_OPTIONS=detect_leaks=0 build/compiler -S -t {p} -o {out_path}')
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

    errno, out, stderr = run_cmd(f'test/MiniVM/build/minivm -t {out_path}', 10, inp)
    out = (out.strip()+'\n'+str(errno)).lstrip()

    if out.strip()!=stdres.strip():
        print('OUTPUT ERROR', p)
        print('OUT: {\n%s\n}'%out)
        print('STD: {\n%s\n}'%stdres)
        1/0
    
    out_path.unlink()
        

print('ALL DONE!')