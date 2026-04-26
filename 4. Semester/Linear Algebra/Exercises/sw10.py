import numpy as np

x= [1.7,1.72,1.86,1.97,1.92,1.8,1.85,1.69,1.77,1.79,1.64,1.82,1.77,1.92,1.82,1.85,1.92,1.71,1.8,1.69] #Body heights
y= [38,38,40,43,42,39,40,37,39,40,37,42,40,41,40,41,41,39,41,40] # Shoes size

# Mean
def mean(l):
    mean = np.sum(l)/len(l)
    return mean

print(mean(x))

# Variance
def variance(l:list)->float:
    sum = 0
    for i in l:
        lc = (i - (mean(l)))**2
        sum += lc
    return sum/(len(l)-1)

print(variance(x))

# Covariance
def covariance(l1:list,l2:list)->float:
    sum = 0
    for i, j in zip(l1,l2):
        sum += (i - mean(l1)) * (j - mean(l2))
    return sum/(len(l1) - 1)

print(covariance(x,y))

# Correlation coefficient
def corrcoeff(l1:list,l2:list)->float:
    cov = covariance(l1,l2)
    sd1 = np.sqrt(variance(l1))
    sd2 = np.sqrt(variance(l2))
    return cov/(sd1*sd2)

print(corrcoeff(x,y))