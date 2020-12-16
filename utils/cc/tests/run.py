#!/usr/bin/python
import os, sys
import subprocess as sp

tests = []
verbose = False
cc_path = os.getcwd() + '/../build/cc'

for arg in sys.argv[1:]:
    if arg in ['-v', '--verbose']:
        verbose = True
    else:
        tests.append(arg)

class TempChdir:
    def __init__(self, path):
        self.old_path = os.getcwd()
        self.path = path

    def __enter__(self):
        os.chdir(self.path)

    def __exit__(self, exc_type, exc_value, traceback):
        os.chdir(self.old_path)

class TempFiles:
    def __init__(self, *paths):
        self.paths = paths

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        for path in self.paths:
            try:
                os.remove(path)
            except:
                pass

def exit_env(code):
    global status_code
    status_code = code
    raise ExitException()

class cc:
    def __init__(self, path):
        if verbose:
            print('Compile', path)
        result = sp.run([cc_path, path], capture_output=True)
        self.cc_errors = str(result.stderr, encoding='UTF-8')
        self.cc_asm = result.stdout
        self.cc_rc = result.returncode
        if verbose:
            print('  Exit status:', self.cc_rc)
            print('  stderr:')
            print(self.cc_errors)

    def no_errors(self):
        if self.cc_rc != 0:
            print(self.cc_errors)
            exit_env(1)

    def has_errors(self):
        if self.cc_rc == 0:
            exit_env(1)

    def _run(self):
        with TempFiles('out.s', 'out.o', 'out'):
            f = open('out.s', 'wb')
            f.write(self.cc_asm)
            f.close()

            result = sp.run(['nasm', 'out.s', '-f', 'elf32', '-o', 'out.o'], capture_output=True)
            if result.returncode != 0:
                exit_env(1)

            result = sp.run(['gcc', 'out.o', '-o', 'out', '-m32'], capture_output=True)
            if result.returncode != 0:
                exit_env(1)

            result = sp.run(['./out'], capture_output=True)
            return result

    def returns(self, val):
        result = self._run()
        if result.returncode != val:
            exit_env(1)

    def outputs(self, expected_file_path):
        result = self._run()
        f = open(expected_file_path, 'rb')
        expected = f.read()
        f.close()

        if result.stdout != expected:
            exit_env(1)

class ExitException(Exception):
    pass

def run_test(path):
    global status_code
    status_code = 0
    with TempChdir(path):
        source = open('run.py').read()
        try:
            exec(source, {
                'cc': cc, 
                'exit': exit_env,
            })
        except ExitException:
            pass
    return status_code == 0

# For each sub-dir in this dir
passed = 0
failed = 0
for entry in os.listdir('./') if len(tests) == 0 else tests:
    if os.path.isdir(entry):
        if run_test(entry):
            print (entry + ': \033[32mpassed\033[m')
            passed += 1
        else:
            print (entry + ': \033[31mfailed\033[m')
            failed += 1

print ('\nResults:')
print (' \033[32mpassed:', passed, '\033[m')
print (' \033[31mfailed:', failed, '\033[m')

