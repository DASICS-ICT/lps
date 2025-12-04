tests = [
    'test_bound.c',
]

foreach t:tests
    test(
        t,
        executable(
            t + '.elf',
            t,
            dependencies: [lps_core]
        ),
        suite: ['core']
    )
endforeach