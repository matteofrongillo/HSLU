# PID Controllers — Detailed Explanation (ChatGPT)

## 1. PID Feedback Law

The controller output is

$$u(t)=K_P e(t) + K_I \int_{0}^{t} e(\tau)\, d\tau + K_D \frac{d}{dt} e(t)$$

where

$$e(t)=r(t)-y(t)$$

- $r(t)$: set-point  
- $y(t)$: measured process value

&nbsp;

### Proportional Term (P)

&nbsp;

- $\displaystyle K_P e(t)$
- Immediate correction proportional to current error
- Reduces rise time
- Cannot remove steady-state error alone
- Too high → oscillation/overshoot

&nbsp;

### Integral Term (I)

&nbsp;

- $\displaystyle K_I \int e(\tau)\,d\tau$
- Eliminates steady-state error
- Slow reaction
- Large integral action → overshoot (integral wind-up)

&nbsp;

### Derivative Term (D)

&nbsp;

- $\displaystyle K_D \frac{d}{dt} e(t)$
- Predictive action: reacts to error rate of change
- Reduces overshoot and oscillations
- Sensitive to noise

&nbsp;

---

&nbsp;

## 2. Set-Point vs Feedback Controllers

&nbsp;

- Pure set-point controllers can overshoot because they don’t consider system response.
- Feedback controllers (P, I, D, PI, PD, PID) adjust the output continuously based on the measured error.

&nbsp;

---

&nbsp;

## 3. Interpretation of Response Curves

### P Controller

&nbsp;

- Medium speed
- Has steady-state error
- May overshoot slightly

&nbsp;

### I Controller

&nbsp;

- Very slow response
- Eventually reaches target (no steady-state error)
- Typically large overshoot

&nbsp;

### PI Controller

&nbsp;

- Faster than pure I
- Eliminates steady-state error
- Mild overshoot

&nbsp;

### PD Controller

&nbsp;

- Fast response
- Low overshoot
- Still has steady-state error

&nbsp;

### PID Controller

&nbsp;

- Fast rise time
- No steady-state error
- Limited overshoot
- Best compromise between accuracy + stability

&nbsp;

---

## 4. Summary Table

| Controller | Rise Time | Overshoot | Steady-State Error | Stability |
| :--- | :--- | :--- | :--- | :--- |
| **P** | Medium | Possible | Yes | Medium |
| **I** | Very Slow | Large | No | Poor |
| **PI** | Fast | Moderate | No | Good |
| **PD** | Fast | Low | Yes | Good |
| **PID** | Fast | Low / Med | No | Very Good |

&nbsp;

---

&nbsp;

## 5. Overshoot Explanation

Overshoot occurs when the system output exceeds the target value before settling.  
It is typically caused by aggressive proportional or integral action.  
PID tuning balances speed, accuracy, and stability to minimize overshoot.