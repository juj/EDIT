import socket, struct, subprocess, os, random, sys

script_directory = os.path.dirname(os.path.abspath(sys.argv[0]))

# Server state
files = {} # document ID -> file contents
filenames = {} # document ID -> filename

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

def receive_datagram(timeout=None):
  global recv_sock
  recv_sock.settimeout(timeout)
  data, addr = recv_sock.recvfrom(1400)
  s = [hex(i) for i in data]
  print("received message: %s, len: %d from %s" % (s, len(data), str(addr)))
  return (data, addr)

def send_datagram(data):
  send_sock.sendto(data, ("255.255.255.255", CLIENT_PORT))

def get_file_lines(document_id):
  if document_id not in files:
    files[document_id] = ['']
  return files[document_id]

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
#      print(l)
      if l.count(':') >= 4:
        parts = l.split(':')
        if len(parts[0]) == 1 and parts[0].isalpha():
          parts = [f'{parts[0]}:{parts[1]}'] + parts[2:]
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
  msg = ascii_to_petscii(error_msg)#error_msg.encode('ascii')
  header = struct.pack('IHBBBB', 0x31268143, line_no, column_no, success, len(msg), 0)
  send_datagram(header + msg)

def n_lower_chars(string):
	return sum(1 for c in string if c.islower())
def n_upper_chars(string):
	return sum(1 for c in string if c.isupper())
def prg_suffix_for_filename(fn):
	if n_upper_chars(fn) >= n_lower_chars(fn): return '.PRG'
	return '.prg'

def calc_output_prg_filename_from_input_cpp_filename(filename):
  if 'untitled_filename' in filename:
    return 'A.OUT.PRG'
  else:
    return replace_suffix(filename, prg_suffix_for_filename(filename))

# contains the Popen object representing the current compilation, if
# one is running on the background, or None if no active compilation.
current_subprocess_build = None
pending_subprocess_build = None # filename to a pending build
build_output_file = None

def compile_file(filename):
  global current_subprocess_build, pending_subprocess_build, build_output_file
  if current_subprocess_build:
    pending_subprocess_build = filename
    return
  pending_subprocess_build = None

  build_output_file = calc_output_prg_filename_from_input_cpp_filename(filename)
  # TODO PATH
  compiler = 'C:/code/llvm-mos-sdk/build/install/bin/mos-c64-clang.bat'
# compiler = 'mos-c64-clang'
  include_dir = os.path.abspath(os.path.join(script_directory, '..', 'C64'))
  additional_libs = [os.path.join(include_dir, 'stdio.cpp')]
#  print(include_dir)
############### XXXXXXXXXXXX DEBUG
  open(filename, 'w').write('''
  #include <stdio.h>
  int main()
  {
    for(;;) printf("hello, world!\\n");
  }''')
  current_subprocess_build = subprocess.Popen([compiler, filename, f'-I{include_dir}', '-Oz', '-o', build_output_file] + additional_libs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

a2p = [
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x14,0x20,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
	0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0x5b,0x5c,0x5d,0x5e,0xa4,
	0xc0,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0xdb,0xdc,0xdd,0xde,0xdf,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x0c,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff]

p2a = [
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x14,0x09,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
	0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x0c,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0x20,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf]

def ascii_to_petscii_ch(ch):
  if ch >= ord('a') and ch <= ord('z'): return 0x41 + (ch - ord('a'))
  if ch >= ord('A') and ch <= ord('Z'): return 0x61 + (ch - ord('A'))
  if ch == ord('_'): return 0xA4
  if ch == ord('{'): return 0xAB
  if ch == ord('}'): return 0xB3
  if ch == ord('\\'): return 0xAE
  return a2p[ch]
'''
	if ch == 0x08: return 0x14
	if ch == 0x0A: return 0x0D
	if ch == 0x0B: return 0x11
	if ch == 0x0C: return 0x93
	if ch == 0x0D: return 0x0A
	if ch == 0x11: return 0x0B
	if ch == 0x14: return 0x08
	if ch == 0x60: return 0xC0
	if ch == 0x93: return 0x0C
	if ch >= 0x41 and ch <= 0x5A: return ch + 0x80
	if ch >= 0x61 and ch <= 0x7A:
		print(f'seeing {ch}')
		return ch - 0x20
	if ch >= 0x7B and ch <= 0x7F: return ch + 0x60
	if ch <= 0xFB or ch >= 0xE0: return ch
	return ch - 0x60
'''

def ascii_to_petscii(s): # shifted PETSCII
	return bytes([ascii_to_petscii_ch(ord(ch)) for ch in s])
'''
p2a = [0]*256
for i in range(256):
	p2a[ascii_to_petscii_ch(i)] = i
'''
#	print(f'PETSCII {ascii_to_petscii_ch(i)} -> ASCII {i} {chr(i)}')

def petscii_to_ascii_ch(ch): # shifted PETSCII
  if ch >= 0x41 and ch <= 0x5A: return ord('a')+(ch-0x41)
  if ch >= 0x61 and ch <= 0x7A: return ord('A')+(ch-0x61)
  if ch == 0xA4: return ord('_')
  if ch == 0xAB: return ord('{')
  if ch == 0xB3: return ord('}')
  if ch == 0xAE: return ord('\\')
  return p2a[ch]
'''
	got = p2a[ch]
	print(f'ch {ch}/{hex(ch)} -> {got}')
	return got
'''

def petscii_to_ascii(s):
	return ''.join([chr(petscii_to_ascii_ch(ch)) for ch in s])

def remove_trailing_null_byte(s):
	if len(s) > 0 and s[len(s)-1] == 0: return s[:len(s)-1]
	return s

MAGIC_UPDATE_LINE_MSG = 0x56369124
MAGIC_NEW_DOCUMENT_MSG = 0x2395173A
MAGIC_ASK_COMPILED_DOCUMENT_MSG = 0x69392AA2

def handle_new_document_msg(data):
  clear_contents, document_id, filename_len, dummy = struct.unpack('BBBB', data[:4])
  petscii_file = remove_trailing_null_byte(data[4:4+filename_len])
  filename = petscii_to_ascii(petscii_file)
  if filename[0] == '<': filename = f'untitled_filename_{document_id}.cpp'

  print(f'New document: id: {document_id} -> clear: {clear_contents}. Filename: {filename}')

  filenames[document_id] = filename
  if clear_contents:
    files[document_id] = ['']
#    print(f'PETSCII filename: {petscii_file} -> ASCII filename: "{filename}"')

def handle_update_line_msg(data):
  line_no, edit_type, document_id, line_len, dummy = struct.unpack('HBBBB', data[:6])
  petscii_line = remove_trailing_null_byte(data[6:6+line_len])
  line = petscii_to_ascii(petscii_line)
#  print(f'PETSCII line: {petscii_line} -> ASCII line: "{line}"')
#  print(f'Line: {line_no}, edit_type: {edit_type}, document_id: {document_id}, line: {line}')
  file_lines = get_file_lines(document_id)

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

  files[document_id] = file_lines
  if document_id not in filenames:
    filenames[document_id] = f'unknown_filename_{document_id}.cpp'

  filename = filenames[document_id]

  save_file(filename, file_lines)
  if filename.lower().endswith('.c') or filename.lower().endswith('.cpp'):
    print_file(file_lines)
    compile_file(filename)


# MAX_C64_FILENAME_LEN 16

RECV_FILE_DATAGRAM_MAGIC = 0x25291423
#struct recv_file_datagram_header
#{
#  uint32_t magic_header;
#  char filename[20];
#  uint16_t file_id;
#  uint16_t num_fragments;         # Actually just a u8, max range 0-254
#  uint16_t file_start_address;
#  uint16_t file_size;
#  uint8_t dummy0;
#  uint8_t filetype;
#};

RECV_FILE_DATAGRAM_FRAGMENT_MAGIC = 0x25291424
#struct recv_file_datagram_fragment_header
#{
#  uint32_t magic_header;
#  uint16_t file_id;
#  uint16_t fragment_index;
#};

def wait_to_ack_datagram_received(datagram_number):
  print(f'wait_to_ack_datagram_received({datagram_number})')
  while True:
    data, addr = receive_datagram()
    print(f'Received {data} from {addr}')
    magic = struct.unpack('I', data[:4])[0]
    data = data[4:]
    FRAGMENT_RECEIVED_DATAGRAM_MAGIC = 0xEE365F02
    if magic == FRAGMENT_RECEIVED_DATAGRAM_MAGIC:
      fragment_index = struct.unpack('BB', data[:2])[0]
      if fragment_index == datagram_number:
        print(f'got ack ({datagram_number})')
        return


def send_file_to_client(filename, crop_first_two_bytes):
  print(f"send_file_to_client({filename})")
  file_data = open(filename, 'rb').read()
  return send_file_data_to_client(filename, file_data, crop_first_two_bytes)


def send_file_data_to_client(filename, file_data, crop_first_two_bytes):
  print(f"send_file_data_to_client({filename})")
  MTU = 1300 ##644
  FRAGMENT_PAYLOAD_SIZE = MTU - 8

  print(str(len(file_data)))
  if len(file_data) % 2:
    file_data += bytes([0]) # Make file lengths always even
  print(str(len(file_data)))
  memory_start_address = struct.unpack('H', file_data[0:2])[0]
  if crop_first_two_bytes:
    file_data = file_data[2:]
  size = len(file_data)
  if size > 512 * FRAGMENT_PAYLOAD_SIZE:
    print(f"File is too large ({size} bytes) to send! Max send size={512 * FRAGMENT_PAYLOAD_SIZE} bytes")
    return;
  print(f'Sending file of size {size}')

  file_id = random.randrange(65536)
  num_fragments = (size + FRAGMENT_PAYLOAD_SIZE - 1) // FRAGMENT_PAYLOAD_SIZE
  filetype = 'S'
  if filename.upper().endswith('.PRG'):
    filetype = 'P'
    filename = filename[:-4]
  out_petscii_filename = ascii_to_petscii(filename.lower())
  recv_file_header = struct.pack('I20sHHHHBB', RECV_FILE_DATAGRAM_MAGIC, out_petscii_filename,
    file_id, num_fragments, memory_start_address, len(file_data), 0, ord(filetype))
  send_datagram(recv_file_header)

  wait_to_ack_datagram_received(0xFF) # Wait for C64 being ready to start receiving actual data

  i = 0
  fragment_index = 0
  while i < size:
    fragment_payload_size = min(FRAGMENT_PAYLOAD_SIZE, size - i)
    fragment_header = struct.pack('IHH', RECV_FILE_DATAGRAM_FRAGMENT_MAGIC, file_id, fragment_index)
    fragment_data = file_data[i:i+fragment_payload_size]
#    print(str(file_data))
    print(f"sending datagram file_id={file_id} fragment_index={fragment_index} i={i}")
    send_datagram(fragment_header + fragment_data)
    wait_to_ack_datagram_received(fragment_index)
    i += FRAGMENT_PAYLOAD_SIZE
    fragment_index += 1
  print('File sent!')

def handle_ask_compiled_document_msg(data):
  print(f"handle_ask_compiled_document_msg({data})")
  document_id, crop_first_two_bytes = struct.unpack('BB', data)
  filename = filenames[document_id]
  out_file = calc_output_prg_filename_from_input_cpp_filename(filename)
  send_file_to_client(out_file, crop_first_two_bytes)

def poll_compilation_finished():
  global current_subprocess_build, pending_subprocess_build, build_output_file
  if not current_subprocess_build: return

  current_subprocess_build.poll()
  if current_subprocess_build.returncode is not None:
    stdout = current_subprocess_build.stdout.read().decode('utf-8')
    stderr = current_subprocess_build.stderr.read().decode('utf-8')
    if current_subprocess_build.returncode == 0:
      print('BUILD OK!')
      send_build_error(0, 0, 1, f'OK. {build_output_file}: {os.path.getsize(build_output_file)} bytes.')
    else:
      line, column, first_error = parse_first_clang_error(stderr)
      print(first_error)
      send_build_error(line, column, 0, first_error)

    current_subprocess_build = None
    if pending_subprocess_build:
      compile_file(pending_subprocess_build)



MAGIC_ASK_AOC_CALENDAR = 0x39DE312B
def handle_ask_aoc_calendar(data):
  print(f"handle_ask_aoc_calendar({data})")
  year = struct.unpack('H', data)[0]
  cmd = ['aoc', 'calendar']
  if year:
    cmd += ['-y', str(year)]
  print(str(cmd))
  output = subprocess.run(cmd, capture_output=True)
  print(str(output.returncode))
  print(str(output.stderr))
  if output.returncode == 0:
    stdout = output.stdout
    print(str(stdout))
    send_file_data_to_client('', stdout, False)


MAGIC_ASK_AOC_DAY = 0x910532EF
def handle_ask_aoc_day_desc(data):
  print(f"handle_ask_aoc_day_desc({data})")
  year, day, column_width = struct.unpack('HBB', data)
  cmd = ['aoc', 'read', '-w', str(column_width)]
  if year:
    cmd += ['-y', str(year)]
  if day:
    cmd += ['-d', str(day)]
  print(str(cmd))
  output = subprocess.run(cmd, capture_output=True)
  print(str(output.returncode))
  print(str(output.stdout))
  print(str(output.stderr))
  if output.returncode == 0:
    stdout = output.stdout
    print(str(stdout))
    send_file_data_to_client('', stdout, False)


MAGIC_ASK_AOC_INPUT = 0x12985331
def handle_ask_aoc_input(data):
  print(f"handle_ask_aoc_input({data})")
  year, day, column_width = struct.unpack('HBB', data)
  cmd = ['aoc', 'download', '--input-only', '--overwrite']
  if year:
    cmd += ['-y', str(year)]
  if day:
    cmd += ['-d', str(day)]
  print(str(cmd))
  output = subprocess.run(cmd, capture_output=True)
  print(str(output.returncode))
  print(str(output.stdout))
  print(str(output.stderr))
  if output.returncode == 0:
    day_inp = open('input', 'rb').read()
    print(str(day_inp))
    send_file_data_to_client('', day_inp, False)



def server_main():
  while True:
    try:
      data, addr = receive_datagram(0.1)
      magic = struct.unpack('I', data[:4])[0]
      data = data[4:]
      if magic == MAGIC_UPDATE_LINE_MSG:
        handle_update_line_msg(data)
      elif magic == MAGIC_NEW_DOCUMENT_MSG:
        handle_new_document_msg(data)
      elif magic == MAGIC_ASK_COMPILED_DOCUMENT_MSG:
        handle_ask_compiled_document_msg(data)
      elif magic == MAGIC_ASK_AOC_CALENDAR:
        handle_ask_aoc_calendar(data)
      elif magic == MAGIC_ASK_AOC_DAY:
        handle_ask_aoc_day_desc(data)
      elif magic == MAGIC_ASK_AOC_INPUT:
        handle_ask_aoc_input(data)
      else:
        print(f'Unknown magic {hex(magic)}')
    except TimeoutError:
      pass
    poll_compilation_finished()



if 'send' in sys.argv:
  fn = sys.argv[-1]
  crop_first_two_bytes = fn.lower().endswith('.prg')
  send_file_to_client(fn, crop_first_two_bytes)
else:
  server_main()
