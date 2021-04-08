from runner import run_cmd, run_cmd_willsucc
import pathlib
from tqdm import tqdm

run_cmd_willsucc('make clean')
print('making compiler')
run_cmd_willsucc('make', 30)

for p in tqdm(sorted(list(pathlib.Path('.').glob('testcases/**/functional*/*.sy')))):
    #print('trying', p)
    out = run_cmd_willsucc(f'build/compiler {p}')

    if not out.rstrip().endswith('TEST PASSED!'):
        print('COMPILE ERROR', p)
        print(out)
        1/0

    eeyore_code = out.partition('///// BEGIN EEYORE')[2].partition('///// END EEYORE')[0]
    basename = p.name.rpartition('.')[0]

    with open(p.with_name(basename+'.out')) as f:
        stdres = f.read().strip()
    if p.with_name(basename+'.in').exists():
        with open(p.with_name(basename+'.in')) as f:
            inp = f.read()
    else:
        inp = ''

    eeyore_path = pathlib.Path('test/%s.eeyore'%basename)
    with eeyore_path.open('w') as f:
        f.write(eeyore_code)

    errno, out, stderr = run_cmd(f'test/MiniVM/build/minivm {eeyore_path}', 10, inp)
    out = (out.strip()+'\n'+str(errno)).lstrip()

    if out.strip()!=stdres.strip():
        print('OUTPUT ERROR', p)
        print('OUT: {\n%s\n}'%out)
        print('STD: {\n%s\n}'%stdres)
        1/0
    
    eeyore_path.unlink()
        

print('ALL DONE!')