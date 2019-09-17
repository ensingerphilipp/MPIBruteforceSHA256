# MPIBruteforceSHA256

Easily Bruteforce SHA256 Hashes in High Performance Cluster Systems - running PelicanHPC

Installation:

1. Setup your PelicanHPC Environment with master and slaves (recommended Size < 50 Systems) 
2. Compile SHA256BruteForceMPI.c and run it on MasterNode using MPIRUN:

Options:
-l Maximum length of bruteforced String e.g.: -l 6 for String <= length 6
-c Charset that is used when bruteforcing e.g.: -c abcdefghijklmnopqrstuvwxyz123
as last argument supply the SHA256 Hash

Example Usage:
mpirun -hostfile -/tmp/bhosts SHA256BruteForceMPI -l 6 -c abcdefghijklmnopqrstuvwxyz e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

========================================

This Project is WiP and needs optimization
