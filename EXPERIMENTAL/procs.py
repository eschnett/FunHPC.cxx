from fractions import Fraction

_hardware = ['node', 'socket', 'core']
_software = ['process', 'thread', 'hyperthread']

class Layout:
    "Map processed and threads onto nodes and cores"
    
    def ___init___(self, hwlimits):
        hwlimits[hw: ...]
        hwlimits[(hw,hw): ...]




int main(int argc, char **argv) {
  if (argc != 3) {
    cerr << "Synopsis: " << argv[0] << " <nodes> <processes>\n";
    return 1;
  }
  // Hardware constraints (cannot be changed)
  const rational<int> max_nodes(1, 0);
  const rational<int> max_sockets_per_node(2);
  const rational<int> max_cores_per_socket(8);
  cout << "Hardware constraints:\n"
       << "   max_nodes:            " << max_nodes << "\n"
       << "   max_sockets_per_node: " << max_sockets_per_node << "\n"
       << "   max_cores_per_socket: " << max_cores_per_socket << "\n";
  // Defaults (could be user input)
  const rational<int> cores_per_socket(max_cores_per_socket);
  const rational<int> sockets_per_node(max_sockets_per_node);
  const rational<int> threads_per_core(1);
  cout << "Defaults:\n"
       << "   cores_per_socket: " << cores_per_socket << "\n"
       << "   sockets_per_node: " << sockets_per_node << "\n"
       << "   threads_per_core: " << threads_per_core << "\n";
  // User choices
  const rational<int> nodes = stoi(argv[1]);
  const rational<int> processes = stoi(argv[2]);
  // Dependent quantities
  const rational<int> sockets = sockets_per_node * nodes;
  const rational<int> cores = cores_per_socket * sockets;
  const rational<int> threads = threads_per_core * cores;
  // Done.
  return 0;
}
