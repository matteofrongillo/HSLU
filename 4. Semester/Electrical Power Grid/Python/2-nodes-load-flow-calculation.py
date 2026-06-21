import cmath

def get_float_input(prompt_text):
    while True:
        try:
            return float(input(prompt_text))
        except ValueError:
            print("Invalid input. Please enter a valid numerical value.")

def get_complex_input(name, real_label, imag_label, unit):
    print(f"\n--- Input {name} ---")
    print("Choose input format:")
    print("1: Rectangular form")
    print("2: Polar form (magnitude + angle in degrees)")

    while True:
        form = input("Choice (1-2): ")
        if form in ['1', '2']:
            break
        print("Invalid selection.")

    if form == '1':
        real = get_float_input(f"Enter {real_label} [{unit}]: ")
        imag = get_float_input(f"Enter {imag_label} [{unit}]: ")
        return complex(real, imag)
    else:
        magnitude = get_float_input(f"Enter magnitude [{unit}]: ")
        angle_deg = get_float_input("Enter angle [deg]: ")
        angle_rad = cmath.pi * angle_deg / 180.0
        return cmath.rect(magnitude, angle_rad)

def get_S_input():
    print("\n--- Input Load Apparent Power (S_B) ---")
    print("Choose input format:")
    print("1: Rectangular form (S = P + jQ)")
    print("2: Polar form (|S| + angle in degrees)")

    while True:
        form = input("Choice (1-2): ")
        if form in ['1', '2']:
            break
        print("Invalid selection.")

    if form == '1':
        P = get_float_input("Enter Active Power P [W]: ")
        Q = get_float_input("Enter Reactive Power Q [var]: ")
        return complex(P, Q)
    else:
        magnitude = get_float_input("Enter Apparent Power |S| [VA]: ")
        angle_deg = get_float_input("Enter phase angle [deg]: ")
        angle_rad = cmath.pi * angle_deg / 180.0
        return cmath.rect(magnitude, angle_rad)

def print_result(name, value, unit):
    magnitude = abs(value)
    angle_deg = cmath.phase(value) * (180.0 / cmath.pi)
    print("\n--- Result ---")
    print(f"{name} (complex) = {value:.4f} {unit}")
    print(f"|{name}| = {magnitude:.4f} {unit}")
    print(f"Angle = {angle_deg:.4f}°")

def main():
    print("Select the variable to solve for in the 2-Node System:")
    print("1: U_B (Node B Voltage)")
    print("2: U_A (Node A Voltage)")
    print("3: Z_AB (Line Impedance)")
    print("4: S_B (Load Apparent Power)")

    while True:
        choice = input("\nChoice (1-4): ")
        if choice in ['1', '2', '3', '4']:
            break
        print("Invalid selection.")

    if choice != '1':
        U_B = get_complex_input("Node B Voltage (U_B)", "Real Part", "Imaginary Part", "V")
    if choice != '2':
        U_A = get_complex_input("Node A Voltage (U_A)", "Real Part", "Imaginary Part", "V")
    if choice != '3':
        Z_AB = get_complex_input("Line Impedance (Z_AB)", "Resistance R", "Reactance X", "Ohms")
    if choice != '4':
        S_B = get_S_input()

    if choice == '1':
        eps = get_float_input("\nEnter convergence threshold epsilon (e.g., 1e-5): ")

        U_B = complex(abs(U_A), 0)
        i = 0
        max_iter = 1000
        converged = False

        while i < max_iter:
            U_B_old = U_B
            I_B = (S_B / U_B).conjugate()
            U_B = U_A - (I_B * Z_AB)

            if abs(abs(U_B) - abs(U_B_old)) < eps:
                converged = True
                break
            i += 1

        if converged:
            print(f"\nConvergence reached after {i+1} iterations.")
            print_result("U_B", U_B, "V")
        else:
            print("\nError: Algorithm did not converge within maximum iterations.")

    elif choice == '2':
        I_B = (S_B / U_B).conjugate()
        U_A_result = U_B + (I_B * Z_AB)
        print_result("U_A", U_A_result, "V")

    elif choice == '3':
        I_B = (S_B / U_B).conjugate()
        try:
            Z_AB_result = (U_A - U_B) / I_B
            print_result("Z_AB", Z_AB_result, "Ohms")
        except ZeroDivisionError:
            print("\nError: Current is zero, cannot determine impedance.")

    elif choice == '4':
        try:
            I_B = (U_A - U_B) / Z_AB
            S_B_result = U_B * I_B.conjugate()
            print_result("S_B", S_B_result, "VA")
        except ZeroDivisionError:
            print("\nError: Impedance is zero, cannot determine power.")

if __name__ == "__main__":
    main()
