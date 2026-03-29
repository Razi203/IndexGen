from optimized_vt import save_vt_to_file, binary_vt_mitm_generator

def build_full_vt_dictionary_optimized(n, a=0, correct_substitutions=True, filename=None):
    """
    Builds the full binary VT code dictionary using Meet-in-the-Middle optimization.
    Saves to filename if provided.
    """
    if filename:
        return save_vt_to_file(n, a, correct_substitutions, filename)
    else:
        # If no filename, return as list (warning: memory intensive for large n)
        return list(binary_vt_mitm_generator(n, a, correct_substitutions))

if __name__ == "__main__":
    # Parameters for the requested large codebook
    n, a = 28, 0 #example values
    correct_substitutions = True
    filename = f"vt_codebook_n{n}_a{a}.txt"
    
    print(f"Starting optimized generation for n={n}...")
    count = build_full_vt_dictionary_optimized(n, a, correct_substitutions, filename)
    print(f"Successfully generated {count} codewords and saved to {filename}")
