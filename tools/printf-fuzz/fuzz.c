#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

int xxc_vsnprintf(char *buffer, size_t size, const char *format, va_list args);

static int xxc_snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int result = xxc_vsnprintf(buffer, size, format, args);
    va_end(args);

    return result;
}

static uint64_t rng_state = 88172645463325252ULL;

static uint64_t random_u64(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;

    return rng_state;
}

static double random_double(void)
{
    switch (random_u64() % 6) 
	{
		case 0: 
		{
			uint64_t bits = random_u64();
			double value;

			memcpy(&value, &bits, sizeof(value));
			return value;
		}

		case 1:
			return (double)(int64_t)(random_u64() % 2000000ULL) /
				(double)(1 + random_u64() % 1000ULL);

		case 2: 
		{
			double mantissa = (double)(random_u64() % (1ULL << 53));
			int exponent = (int)(random_u64() % 200ULL) - 100;

			return ldexp(mantissa, exponent);
		}

		case 3: 
		{
			int exponent = (int)(random_u64() % 600ULL) - 300;
			double fraction = 1.0 + (random_u64() % 1000ULL) / 1000.0;

			return pow(10.0, exponent) * fraction;
		}

		case 4:
			return (double)(random_u64() % 100000ULL) /
				(double)(1U << (random_u64() % 12ULL));

		default: 
		{
			double value = (double)(random_u64() % 1000ULL) + 0.5;

			return (random_u64() % 2ULL) ? value : -value;
		}
    }
}

static long double random_long_double(void)
{
    switch (random_u64() % 5) 
	{
		case 0: 
		{
			unsigned char bytes[16] = {0};
			uint64_t mantissa = random_u64() | (1ULL << 63);
			uint16_t sign_exponent = (uint16_t)(1 + random_u64() % 0x7FFDULL);
			if (random_u64() % 2ULL) sign_exponent |= 0x8000;

			memcpy(bytes, &mantissa, sizeof(mantissa));
			memcpy(bytes + 8, &sign_exponent, sizeof(sign_exponent));

			long double value;
			memcpy(&value, bytes, sizeof(value));

			return value;
		}

		case 1: 
		{
			unsigned char bytes[16] = {0};
			uint64_t mantissa = random_u64() & ~(1ULL << 63);
			uint16_t sign_exponent = 0;
			if (random_u64() % 2ULL) sign_exponent = 0x8000;

			memcpy(bytes, &mantissa, sizeof(mantissa));
			memcpy(bytes + 8, &sign_exponent, sizeof(sign_exponent));

			long double value;
			memcpy(&value, bytes, sizeof(value));

			return value;
		}

		case 2: 
		{
			long double mantissa = (long double)random_u64();
			int exponent = (int)(random_u64() % 400ULL) - 200;

			return ldexpl(mantissa, exponent);
		}

		case 3:
			return (long double)random_double();

		default: 
		{
			int exponent = (int)(random_u64() % 9000ULL) - 4500;
			long double fraction = 1.0L + (random_u64() % 1000ULL) / 1000.0L;

			return powl(10.0L, exponent) * fraction;
		}
    }
}

static const char *const format_flags[] = {
    "", "-", "+",
    " ", "#", "0",
    "-+", "+0", "# ",
	"#0", "-#",  " 0"
};

static const char conversions[] = "feEgGaAF";

static void build_format(char *buffer, size_t buffer_size, const char *flags,
    int width, int precision, int use_long_double, char conversion
)
{
    int written = snprintf(buffer, buffer_size, "%%%s", flags);

    if (width >= 0 && written < (int)buffer_size)
        written += snprintf(buffer + written, buffer_size - (size_t)written, "%d", width);

    if (precision >= 0 && written < (int)buffer_size)
        written += snprintf(buffer + written, buffer_size - (size_t)written, ".%d", precision);

    if (use_long_double && written < (int)buffer_size - 1)
        buffer[written++] = 'L';

    if (written < (int)buffer_size - 1)
        buffer[written++] = conversion;

    buffer[written] = '\0';
}

static void print_long_double_mismatch(const char *format, long double value,
    const char *reference_output, const char *test_output
)
{
    unsigned char bytes[16];
    uint64_t mantissa;
    uint16_t sign_exponent;

    memcpy(bytes, &value, sizeof(bytes));
    memcpy(&mantissa, bytes, sizeof(mantissa));
    memcpy(&sign_exponent, bytes + 8, sizeof(sign_exponent));

    printf("BITS m=%016llx se=%04x\n", (unsigned long long)mantissa, sign_exponent);
    printf("MISMATCH %s ld=%Lg glibc='%.200s' ours='%.200s'\n", format, value, reference_output, test_output);
}

static void print_double_mismatch(const char *format, double value, const char *reference_output, 
	const char *test_output
)
{
    printf("MISMATCH %s v=%.17g glibc='%.200s' ours='%.200s'\n", format, value, reference_output, test_output);
}

static int test_long_double_format(const char *format, long double value, char *reference_buffer,
    char *test_buffer, size_t buffer_size, long *mismatch_count
)
{
    int reference_result = snprintf(reference_buffer, buffer_size, format, value);

    int test_result = xxc_snprintf(test_buffer, buffer_size, format, value);

    if (reference_result == test_result && 
		strcmp(reference_buffer, test_buffer) == 0
	)
	{
        return 0;
    }

    ++(*mismatch_count);
    if (*mismatch_count <= 15)
        print_long_double_mismatch(format, value, reference_buffer, test_buffer);

    return 1;
}

static int test_double_format(const char *format, double value, char *reference_buffer,
    char *test_buffer, size_t buffer_size, long *mismatch_count
)
{
    int reference_result = snprintf(reference_buffer, buffer_size, format, value);
    int test_result = xxc_snprintf(test_buffer, buffer_size, format, value);

    if (reference_result == test_result && 
		strcmp(reference_buffer, test_buffer) == 0
	) 
	{
        return 0;
    }

    ++(*mismatch_count);

    if (*mismatch_count <= 15)
        print_double_mismatch(format, value, reference_buffer, test_buffer);

    return 1;
}

int main(int argc, char **argv)
{
    const long default_iterations = 1000000L;
    const size_t buffer_size = 40000;
    long iterations = (argc > 1) ? atol(argv[1]) : default_iterations;
    if (argc > 2) rng_state ^= (uint64_t)atol(argv[2]) * 0x9E3779B97F4A7C15ULL;

    static char reference_buffer[40000];
    static char test_buffer[40000];
    long mismatch_count = 0;

    for (long iteration = 0; iteration < iterations; ++iteration)
	{
        char format[64];
        int use_long_double = (random_u64() % 4ULL) == 0;
        char conversion = conversions[random_u64() % 8ULL];
        const char *flags = format_flags[random_u64() % 12ULL];

        int width = -1;
        if ((random_u64() % 3ULL) == 0)
            width = (int)(random_u64() % 40ULL);

        int precision = -1;
        if ((random_u64() % 3ULL) == 0)
		{
            uint64_t limit = (random_u64() % 8ULL) == 0 ? 400ULL : 25ULL;

            precision = (int)(random_u64() % limit);
        }

        build_format(format, sizeof(format), flags, width, precision, use_long_double, conversion);

        if (use_long_double)
		{
            long double value = random_long_double();

            test_long_double_format(format, value, reference_buffer, test_buffer, buffer_size, &mismatch_count);
        }
		else
		{
            double value = random_double();

            test_double_format(format, value, reference_buffer, test_buffer, buffer_size, &mismatch_count);
        }
    }

    printf("%ld iterations, %ld mismatches\n", iterations,mismatch_count);

    return mismatch_count != 0;
}