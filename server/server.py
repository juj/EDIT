import socket, struct, subprocess, os

recv_sock = None
send_sock = None
SERVER_PORT = 4009
CLIENT_PORT = 4008

def replace_suffix(filename, new_ext):
  if new_ext[0] != '.': new_ext = '.' + new_ext
  return os.path.splitext(filename)[0] + new_ext

def init_network():
  global recv_sock, send_sock
  UDP_IP = ""

  recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
  recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  recv_sock.bind((UDP_IP, SERVER_PORT))

  send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
  send_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  send_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

init_network()

def receive_datagram():
  global recv_sock
  data, addr = recv_sock.recvfrom(1400)
  #print("received message: %s, len: %d" % (data, len(data)))
  return (data, addr)

def send_datagram(data):
  send_sock.sendto(data, ("255.255.255.255", CLIENT_PORT))

files = {}
def get_file_lines(filename):
  if filename not in files:
    files[filename] = ['']
  return files[filename]

def print_file(lines):
  line_no = 0
  for l in lines:
    print(f'{line_no}: {l}')
    line_no += 1

def save_file(filename, file_lines):
  f = open(filename, 'w')
  for l in file_lines:
    f.write(l + '\n')
  f.close()

def parse_first_clang_error(stderr):
  lines = stderr.split('\n')
  for l in lines:
    if 'error:' in l:
      if l.count(':') >= 4:
        parts = l.split(':')
        file = parts[0]
        line = int(parts[1])
        column = int(parts[2])
      else:
        file = 0
        line = 0
        column = 0
      error = l[l.find('error:')+len('error:'):]

      error = error.replace('after top level declarator', '')
      error = error.replace('main cannot be declared as global variable', 'main cannot be a variable')
      error = error.replace('expected function body after function declarator', 'expected function body')
      error = error.replace('use of undeclared identifier', 'undeclared')
      error = error.replace('did you mean', 'maybe')
#      error = error.replace('cannot initialize return object of type', 'cannot return')
      error = error.replace(' with an lvalue of type ', 'lvalue=')
      error = error.replace('a type specifier is required for all declarations', 'a type specifier is required')

      error = error.strip()
      if len(error) > 34:
        error = error[:32] + '..'
      return (line, column, error)

def send_build_error(line_no, column_no, success, error_msg):
  msg = error_msg.encode('ascii')
  header = struct.pack('IHBBB', 0x31268143, line_no, column_no, success, len(msg))
  send_datagram(header + msg)

def compile_file(filename):
  # TODO PATH
  if 'untitled_filename' in filename:
    out_file = 'A.PRG'
  else:
    out_file = replace_suffix(filename, '.PRG')
  clang = subprocess.run(['C:/code/llvm-mos-sdk/build/install/bin/mos-c64-clang.bat', filename, '-Oz', '-o', out_file], capture_output=True)
  if clang.returncode == 0:
    print('BUILD OK!')
    filesize = os.path.getsize(out_file)
    send_build_error(0, 0, 1, f'BUILD OK. {out_file}: {filesize} BYTES.')
  else:
    line, column, first_error = parse_first_clang_error(clang.stderr.decode('utf-8'))
    print(first_error)
    send_build_error(line, column, 0, first_error)

while True:
    in_blocking_mode = 0
#  try:
    data, addr = receive_datagram()
    #print(str(data))
    magic, line_no, edit_type, filename_len, line_len = struct.unpack('IHBBB', data[:9])
    if magic != 0x56369124: continue # Not a datagram for us
    filename = data[9:9+filename_len].decode('ascii')
    line = data[9+filename_len:9+filename_len+line_len].decode('ascii')
    #print(f'Line: {line_no}, edit_type: {edit_type}, filename: {filename}, line: {line}')
    file_lines = get_file_lines(filename)

    if edit_type == 0: # Insert
#      print(f'INSERT {line_no} <- {line}')
      file_lines = file_lines[:line_no] + [line] + file_lines[line_no:]
    elif edit_type == 1: # Replace
#      print(f'REPLACE {line_no} <- {line}')
#      print(str(file_lines))

      while line_no >= len(file_lines):
        file_lines += ['']

      file_lines[line_no] = line
    elif edit_type == 2: # Delete
#      print(f'DELETE {line_no}')
      file_lines = file_lines[:line_no] + file_lines[line_no+1:]
    else:
      raise Exception(f'invalid edit type {edit_type}')

    files[filename] = file_lines

    if filename[0] == '<': filename = 'untitled_filename.cpp'
    save_file(filename, file_lines)
    if filename.lower().endswith('.c') or filename.lower().endswith('.cpp'):
      print_file(file_lines)
      compile_file(filename)
#  except Exception as e:
#    print(str(e))


