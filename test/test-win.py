from runner import run_cmd_willsucc
import pathlib
from tqdm import tqdm

for p in tqdm(list(pathlib.Path('.').glob('testcases/**/*.sy'))):
    #print('trying', p)
    out = run_cmd_willsucc(f'cmake-build-debug\\compiler.exe {p}')
    if not out.rstrip().endswith('TEST PASSED!'):
        print('RESULT ERROR', p)
        print(out)
        1/0

print('ALL DONE!')