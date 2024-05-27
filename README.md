# random_sendrecv

This program uses MPI's Isend, Irecv to perform random P2P communications. The amount of communication is also random. However, each rank sends and receives the same data size to its peer. To achieve this, a traceless target matrix is first created as a communication table.
This program was developed to validate a tool that optimizes communication in unstructured lattice applications by the placement of ranks.

## Usage

mpiexec ./random_sendrecv <max peers> <min data size> <max data size> <seed for random number>
