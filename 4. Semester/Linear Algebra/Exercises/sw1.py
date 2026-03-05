# 0.2
import numpy as np


list=[a,b,c,d,e,f]=[2/3*np.pi/(np.e*5/(11*7)), 1+1/(2+2/(3+(4/(4+8/5)))), (np.pi**2+2**(3/4))/(np.e-5/7), np.pi**(np.sqrt(2)*(17*np.sqrt(3)/(20*np.cbrt(2))**(1+(4/17)**(2/3)))), np.e**np.e**np.e**(-np.e), np.arctan(np.sqrt(1+0.7**2)/(np.sqrt(1-0.7**2)))]
print("Exercise 2:")
for _ in list:
    print(str(_)[:6])

# 0.3
m,v,x = 1790, 110/3.6, 270
def P(m,v,x):
    t = x/v
    return 0.5*m*v**2/t
print(f"\nExercise 3:\nP = {P(m,v,x)/1000:.1f} kW")

# 0.4
load,snow,rho = 1500,5E-2,200
def f(load,rho,snow):
    g = 9.81
    s_max = load/(rho*g)
    return s_max/snow
print(f"\nExercise 4:\nf = {np.floor(f(load,rho,snow)):.0f}h{np.floor((f(load,rho,snow)-np.floor(f(load,rho,snow)))*60):.0f}min")

# 0.5
