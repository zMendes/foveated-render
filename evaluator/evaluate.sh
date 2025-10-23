#!/bin/zsh

cd ..
cd build/

# Baselines
./my_program NO reference > /dev/null 2>&1
./my_program STATIC static > /dev/null 2>&1
./my_program PADDED padding > /dev/null 2>&1

# Grid search parameters
alphas=(0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5)
betas=(0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0)

for alpha in "${alphas[@]}"; do
  for beta in "${betas[@]}"; do
    name="adaptive_${alpha}_${beta}"
    echo "Running my_program ADAPTIVE $name $alpha $beta"
    ./my_program ADAPTIVE "$name" "$alpha" "$beta" > /dev/null 2>&1
  done
done

cd ../fovvideovdp
source .env/bin/activate

# Run result.py for each generated output
python3 result.py reference static
python3 result.py reference padding

for alpha in "${alphas[@]}"; do
  for beta in "${betas[@]}"; do
    name="adaptive_${alpha}_${beta}"
    python3 result.py reference "$name"
  done
done
