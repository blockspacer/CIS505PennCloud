This folder is for code that will be useful for multiple components

.proto files will be used to generate corresponding .h and .cc files

There is a sample program that uses the UserAccountDTO object created from UserAccountDTO.proto.
It creates a UserAccountDTO and writes it to disk. It then reads it back in from disk.
Use make target 'sample' to compile this program.