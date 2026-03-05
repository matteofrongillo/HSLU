import numpy as np

# Exercise 3.8

# a
M = np.array([[1,7],[4,2]])
N = np.array([[6,0],[0,5]])
P = -np.pi * np.ones((2,2))
Q = np.array([[np.e + np.sqrt(2), np.e],[np.e, np.e - np.sqrt(2)]])
R = np.array([[M,N],[P,Q]])
S = np.array([[M+N, np.zeros((2,2))],[np.zeros((2,2)), P/np.pi]])

#print(f"M:\n{M}")
#print(f"N:\n{N}")
#print(f"P:\n{P}")
#print(f"Q:\n{Q}")
#print(f"R:\n{R}")
#print(f"S:\n{S}")

# b
#print(f'R+S:\n{R+S}')
#print(f"R':\n{R.T}")
#print(f'MN:\n{M@N}')
#print(f'NM:\n{N@M}')

# c
#print(M*N)
#print(M**N)

# d
print(S[1:4])
print(R[0:2])
print(R[1:4:-1])