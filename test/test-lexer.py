# usage in main:
# test_lexer(argv[1]);

from runner import run_cmd_willsucc
import pathlib
from tqdm import tqdm

run_cmd_willsucc('make clean')
run_cmd_willsucc('make')

for p in tqdm(list(pathlib.Path('.').glob('testcases/**/*.sy'))):
    print('trying', p)
    out = run_cmd_willsucc(f'build/compiler {p}')
    if not out.rstrip().endswith('TEST PASSED!'):
        print('RESULT ERROR', p)
        #print(out)
        #1/0

print('ALL DONE!')