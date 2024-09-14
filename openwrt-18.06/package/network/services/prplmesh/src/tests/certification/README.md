# Passing certification tests

## R1

prplMesh passed all the R1 (wired and wireless) certification tests, except the following ones which we are not planning to support.

Unsupported tests:

- Tests which require three radios:
  MAP-4.7.1_BH5GH_FH5GL
  MAP-4.7.2_BH5GH_FH5GL
  MAP-4.10.5_BH5GH_FH5GL
  MAP-4.10.5_BH5GL_FH5GH
  MAP-4.10.6_BH5GH_FH5GL
  MAP-4.10.6_BH5GL_FH5GH

- Tests for some optional, unsupported features:
  MAP-4.7.8_BHWIFI_FH24G
  MAP-4.7.8_BHWIFI_FH5GH
  MAP-4.7.8_BHWIFI_FH5GL
  MAP-4.7.8_ETH_FH24G
  MAP-4.7.8_ETH_FH5GH
  MAP-4.7.8_ETH_FH5GL
  MAP-4.8.4_BHWIFI_FH24G
  MAP-4.8.4_BHWIFI_FH5GH
  MAP-4.8.4_BHWIFI_FH5GL
  MAP-4.8.4_ETH_FH24G
  MAP-4.8.4_ETH_FH5GH
  MAP-4.8.4_ETH_FH5GL

### Generating lists of passing tests

The total amount of time needed to run all the certification tests prplMesh is passing is too big to be able to run them every night.

Because of this, the list of passing tests has been split into two: variant A and variant B.

To make a change to the list of passing tests, *do not edit the variants files by hand*: instead, edit `passing_tests.txt` and re-generate the variants files.

To re-generate the variant files, run the following snippet from the top level directory:
``` sh
# Variant A:
{
  echo "# GENERATED FILE - DO NOT EDIT. See tests/certification/README.md"
  awk 'NR%2==0' tests/certification/passing_tests.txt &&
  printf 'MAP-4.2.1\nMAP-5.3.1\nMAP-5.5.1\nMAP-5.6.1\nMAP-5.7.1\nMAP-5.10.1\nMAP-5.10.2\n';
} | sort -V -u > tests/certification/passing_tests_variant_A.txt

# Variant B:
{
  echo "# GENERATED FILE - DO NOT EDIT. See tests/certification/README.md"
  awk 'NR%2==1' tests/certification/passing_tests.txt &&
  printf 'MAP-4.2.1\nMAP-5.3.1\nMAP-5.5.1\nMAP-5.6.1\nMAP-5.7.1\nMAP-5.10.1\nMAP-5.10.2\n';
} | sort -V -u > tests/certification/passing_tests_variant_B.txt
```
