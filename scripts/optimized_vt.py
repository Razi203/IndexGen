import numpy as np
import time

def binary_vt_mitm_generator(n, a, correct_substitutions=True):
    """
    Generator for binary VT codewords using Meet-in-the-Middle.
    Complexity: O(2^(n/2) * m)
    """
    m = (2 * n + 1) if correct_substitutions else (n + 1)
    
    n1 = n // 2
    n2 = n - n1
    
    # Map: syndrome_sum % m -> list of partial integers
    left_side = {}
    
    # Precompute left side: bits 1 to n1
    for i in range(1 << n1):
        s1 = 0
        for j in range(n1):
            if (i >> j) & 1:
                # Bit j corresponds to position j+1
                s1 += (j + 1)
        
        mod_s1 = s1 % m
        if mod_s1 not in left_side:
            left_side[mod_s1] = []
        left_side[mod_s1].append(i)
        
    # Process right side: bits n1+1 to n
    for i in range(1 << n2):
        s2 = 0
        for j in range(n2):
            if (i >> j) & 1:
                # Bit j corresponds to position n1 + j + 1
                s2 += (n1 + j + 1)
        
        target = (a - s2) % m
        if target in left_side:
            # Combine all matching left halves with this right half
            for left_val in left_side[target]:
                # Combine: Left bits are lower n1 bits, Right bits are upper n2 bits
                # Actually, bits are just bits. 
                # left_val: bits[0...n1-1]
                # i: bits[n1...n-1]
                yield (i << n1) | left_val

def save_vt_to_file(n, a, correct_substitutions, filename):
    """
    Saves binary VT codebook to a file, one word per line.
    """
    count = 0
    start_time = time.time()
    passed_time = start_time
    
    print(f"Generating binary VT codebook (n={n}, a={a}, correct_substitutions={correct_substitutions})...")

    with open(filename, 'w', buffering=1<<20) as f: # 1MB buffer
        
        for codeword_int in binary_vt_mitm_generator(n, a, correct_substitutions):
            # Convert integer to binary string of length n
            # {0:0{n}b} formats integer as binary with length n, zero padded
            binary_str = format(codeword_int, f'0{n}b')[::-1] # Reverse because j-th bit maps to j+1 pos
            # Wait, in the generator: (i << n1) | left_val
            # left_val is n1 bits. i is n2 bits.
            # bits [0...n1-1] are from left_val
            # bits [n1...n-1] are from i
            # the bits in left_val: bit j corresponds to position j+1
            # the bits in i: bit j corresponds to position n1+j+1
            # So binary_str should be: 
            # bits 0..n1-1 from left_val (LSB to MSB)
            # bits n1..n-1 from i (LSB to MSB)
            
            # format(codeword_int, f'0{n}b') gives MSB first (bit n-1 at index 0)
            # We want position 1 (bit 0) at index 0.
            # So we reverse.
            f.write(binary_str + '\n')
            count += 1

            if time.time() - passed_time > 60:
                print(f"Generated {count} codewords in {time.time() - start_time:.2f} seconds.")
                passed_time = time.time()
                
            
            

    end_time = time.time()
    print(f"Done! Generated {count} codewords in {end_time - start_time:.2f} seconds.")
    print(f"Codebook saved to: {filename}")
    return count

if __name__ == "__main__":
    # Test with n=14, a=0
    import vt
    save_vt_to_file(14, 0, True, "vt_n14_a0.txt")
