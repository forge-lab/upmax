#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <vector>

using namespace std;

typedef struct {
  int mem;
  int cpu;
  int used_mem;
  int used_cpu;
} ServerType;

typedef struct {
  int mem;
  int cpu;
  int location;
} VMType;

int _nVMs;
int _nServers;
float _percentageResources;
int _K;

list<VMType*> _vms;
vector<ServerType*> _servers;
int _total_mem;
int _total_cpu;

int _nMem = 3;
int _mem[] = { 32, 64, 128 };
int _nVMMem = 7;
int _VMmem[] = { 2, 4, 6, 8, 10, 12, 16 };
int _nCPU = 4;
int _cpu[] = { 16, 24, 32, 48 };
int _nVMcpu = 4;
int _VMcpu[] = { 1, 2, 4, 6 };

// Generates a random int between 0 and max
int randomInt(int max) {
  return (rand() % (max+1));
}


int randomMem() {
  return _mem[randomInt(_nMem-1)];
}

int randomVMMem() {
  return _VMmem[randomInt(_nVMMem-1)];
}


int randomCPU() {
  return _cpu[randomInt(_nCPU-1)];
}

int randomVMCPU() {
  return _VMcpu[randomInt(_nVMcpu-1)];
}


// Initializes servers
void initServers() {
  while (_servers.size() < _nServers) {
    // Create a new server
    ServerType* s = new ServerType;
    s->mem = randomMem();
    s->cpu = randomCPU();
    s->used_mem = 0;
    s->used_cpu = 0;
    _total_mem += s->mem;
    _total_cpu += s->cpu;
    _servers.push_back(s);
  }
}


// Places VMs
void placeVMs() {
  int usage_mem = 0, usage_cpu = 0;
  
  while (usage_mem < _percentageResources*_total_mem &&
	 usage_cpu < _percentageResources*_total_cpu) {
    VMType* vm = new VMType;
    vm->mem = randomVMMem();
    vm->cpu = randomVMCPU();
    vm->location = 0;
    for (int i = 0; i < _servers.size(); i++) {
      ServerType * s = _servers[i];
      if (s->mem - s->used_mem >= vm->mem && s->cpu - s->used_cpu >= vm->cpu) {
	// VM is placed in this server...
	usage_mem += vm->mem;
	usage_cpu += vm->cpu;
	s->used_mem += vm->mem;
	s->used_cpu += vm->cpu;
	vm->location = i+1;
	break;
      }
    }
    if (vm->location == 0) delete vm;
    else {
      _vms.push_back(vm);
      _nVMs++;
    }
  }
  //printf("Total Mem: %d\tUsed Mem: %d\n", _total_mem, usage_mem);
  //printf("Total CPU: %d\tUsed CPU: %d\n", _total_cpu, usage_cpu);
  _K = 0;
  for (int i = 0; i < _servers.size(); i++) {
    ServerType * s = _servers[i];
    if (s->used_mem > 0)
      _K++;
  }
}


// Shuffle VMs. Tries to spread the VMs among all servers.
void shuffleVMs() {
  for (list<VMType*>::iterator iter = _vms.begin(); iter != _vms.end(); iter++) {
    VMType* vm = *iter;
    int oldLocation = vm->location;
    ServerType * s_old = _servers[oldLocation-1];
    
    // Check if it is the only VM in the server. In this case, do not move it.
    if (s_old->used_mem == vm->mem) continue;
    
    int newLocation = randomInt(_nServers-1)+1;
    while (newLocation == oldLocation)
      newLocation = randomInt(_nServers-1)+1;
    
    //printf("New location: %d\n", newLocation-1);
    
    ServerType * s_new = _servers[newLocation-1];
    
    if (s_new->mem - s_new->used_mem >= vm->mem && s_new->cpu - s_new->used_cpu >= vm->cpu) {
      // VM is moved
      s_new->used_mem += vm->mem;
      s_new->used_cpu += vm->cpu;
      s_old->used_mem -= vm->mem;
      s_old->used_cpu -= vm->cpu;
      vm->location = newLocation;
    }
  }
}



void print() {
  printf("%d\n", _nServers);
  for (int i = 0; i < _servers.size(); i++)
    printf("%d %d\n", _servers[i]->mem, _servers[i]->cpu);
  printf("%d\n", _nVMs);
  for (list<VMType*>::iterator iter = _vms.begin(); iter != _vms.end(); iter++)
    printf("%d %d %d\n", (*iter)->mem, (*iter)->cpu, (*iter)->location);
  printf("%d\n", _K);
}


void printHelp() {
  printf("\n\nUsage: vmc-generator #Servers PercentageUsage seed\n\n");
  printf("#Servers: Number of servers in the instance.\n");
  printf("PercentageUsage: Percentage of usage of any resource.\n");
  printf("seed: (optional) Random seed initializer (default: 1).\n\n");
  printf("Example: ./vmc-generator 5 50\n");
  printf("Creates an instance with 5 servers and stops creating VMs when one resource (mem or cpu) reaches 50%%.\n\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  _nVMs = 0;
  if (argc < 3) printHelp();
  _nServers = atoi(argv[1]);
  _percentageResources = atoi(argv[2]) / 100.0;
  
  // Initialize random seed
  if (argc > 3)
    srand(atoi(argv[3]));
  else
    srand(1); // default seed
  
  initServers();
  
  placeVMs();
  
  shuffleVMs();
  
  print();
  
  return 0;
}

