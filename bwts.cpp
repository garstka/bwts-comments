/*
    BWT by Mark Nelson, 1996
    BWTS by David Scott, 2007

    Comments and minor cosmetic changes
    by Matt Garstka, 2016

    Usage:
    bwts in.bin out.bin
*/

// #define unix

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#if !defined(unix)
#include <io.h>
#endif
#include <limits.h>
#if (INT_MAX == 32767)
#define BLOCK_SIZE 20000
#else
#define BLOCK_SIZE 1000000
#endif

#include <cstdint>

// Block of characters read from a file.
unsigned char buffer[BLOCK_SIZE];

// Indices within "buffer" that get sorted (more on that later).
unsigned int indices[BLOCK_SIZE];

/*
    Array of cycles, formerly xx.

    For all i, buffer[i] can be factorized to be a member
    of a unique cycle of contiguous elements.

    If the cycle indices are (i,i+1,...,i+m),
    the value of next[i+n], n in {0,1,..,m}
    is given by:

    If n == m, then
      next[i+n] = i // Last element loops around to the first.
    otherwise
      next[i+n] = min j { (i+n < j < i+m &&
                           buffer[i+n] != buffer[j]) // Skip runs of same char.
                         || j == i+m } // But if impossible, point to the last.

*/
unsigned int next[BLOCK_SIZE];

/*
    Array of the last chars in the cycle rotations starting at the given
    index, formerly bufs.


    If the cycle indices are (i,i+1,...,i+m),
    the value of last[i+n], n in {0,1,..,m}
    is given by:

    If n == 0, then
      last[i+n] = buffer[i+m] // Last char of this rotation will be at i+m.
    otherwise
      last[i+n] = buffer[i+n-1] // Rotation will wrap around.

*/
unsigned char last[BLOCK_SIZE];


void dumpit(size_t length)
{
	int i;

	fprintf(stderr, "\n indices \n");
	for (i = 0; i < length; i++)
		fprintf(stderr, "<%d>", indices[i]);

	fprintf(stderr, "\n next \n");
	for (i = 0; i < length; i++)
		fprintf(stderr, "<%x>", next[i]);

	fprintf(stderr, "\n partition bottom \n");
	for (i = 0; i < length; i++)
		if (next[i] <= i)
			fprintf(stderr, "<%x>", i);

	fprintf(stderr, "\n buffer \n");
	for (i = 0; i < length; i++)
		fprintf(stderr, "<%x>", buffer[i]);

	fprintf(stderr, "\n last \n");
	for (i = 0; i < length; i++)
		fprintf(stderr, "<%x>", last[i]);

	fprintf(stderr, "\n");
}

/*
    Fills "next" as if the whole buffer was a single cycle, that is:

    For all i in {start,start+1,...,end} the value of buffer[i] is given by:

    If i == end
        next[i] = 0
    else if i != end && there exists j > i such that buffer[j] != buffer[i]
        next[i] = j
    else
        next[i] = end

    Example: for mississippi, next is 1 2 4 4 5 7 7 8 10 10 0
             for mississippiii, next is 1 2 4 4 5 7 7 8 10 10 13 13 0
*/
void Fill_bufs(int start, int end)
{
	// Scott:
	/* called after new buff of data */
	/* tg is the length of buffer */

	auto last_char = buffer[end];
	auto last_jump = static_cast<unsigned int>(end);

	for (int i = end; i >= start; i--)
	{
		auto current_char = buffer[i];
		if (current_char != last_char)
		{
			last_char = current_char;
			last_jump = i + 1;
		}
		next[i] = last_jump;
	}
	next[end] = start;
}


/*
    Fixes @c next according to the rules of Fill_bufs,
    starting from @c end and working backwards. If
    the encountered element already has the correct value,
    returns.
*/
void Mod_bufs(int start, int end)
{
	// Scott:
	/* called after new buff of data */
	/* tg is the length of buffer */

	auto last_char = buffer[end];
	auto last_jump = static_cast<unsigned int>(end);

	for (int i = end; i >= start; i--)
	{
		auto current_char = buffer[i];
		if (current_char != last_char)
		{
			last_char = current_char;
			last_jump = i + 1;
		}

		/*
		    The difference to Fill_bufs: break if correct jump found.
		*/
		if (next[i] == last_jump)
			break;

		next[i] = last_jump;
	}
	next[end] = start;
}

/*
    Lexicographically compares two infinite cyclic sequences of strings,
    respectively:
        i ~ I: the rotation starting with buffer[i]
               of (buffer[k],buffer[k+1],...,buffer[i],...,buffer[l])
               where next[l]==k,
        j ~ J: the rotation starting with buffer[j]
               of (buffer[m],buffer[m+1],...,buffer[j],...,buffer[n])
               where next[n]==m

    Return values:
        If I < J, returns -1.
        If I = J, returns 0
        If I > J, returns 1.
*/
int
#if defined(_MSC_VER)
    _cdecl
#endif
    bounded_compare(const unsigned int* i1, const unsigned int* i2)
{
	auto i = *i1;
	auto j = *i2;

	/*
	    Wrap counters to detect equality.
	*/
	auto i_wraps_left = uint8_t(3);
	auto j_wraps_left = uint8_t(3);

	if (buffer[i] != buffer[j])
		return buffer[i] < buffer[j] ? -1 : 1;

	for (;;)
	{
		// Scott:
		/* next find which has smallest jump */

		// Shortest jump means the next index to lexicographically compare.

		if (i < next[i]) // If the string I won't wrap around yet
		{
			if (j < next[j]) // If the string J won't wrap around yet
			{
				// If I has less repetitions of a single char than J
				if ((next[i] - i) < (next[j] - j))
				{
					// Compare next[i] with the repeating char in J.
					j += next[i] - i;
					i = next[i];
				}
				else if ((next[i] - i) > (next[j] - j)) // If I has more
				                                        // repetitions of a
				                                        // single char than J
				{
					// Compare next[j] with the repeating char in I.
					i += next[j] - j;
					j = next[j];
				}
				else // Either no repeating char or the same count of repeats.
				{
					i = next[i]; // Just compare next[i] with next[j]
					j = next[j];
				}
			}
			else // The string J wraps around now.
			{
				if (j_wraps_left != 0) // J wrapped around.
					j_wraps_left--;

				j = next[j]; // Wrap it.
				i++; // Compare directly with the next in I (don't skip any
				     // repeats).
			}
		}
		else
		{
			if (j < next[j]) // If the string J won't wrap around yet
			{
				j++; // Compare directly with the next in J (don't skip any
				     // repeats).
			}
			else // The string J wraps around as well.
			{
				if (j_wraps_left != 0) // J wrapped around.
					j_wraps_left--;

				j = next[j]; // Wrap it.
			}

			i = next[i]; // Wrap I.

			if (i_wraps_left != 0) // I wrapped around.
				i_wraps_left--;
		}

		// If I and J differ here.
		if (buffer[i] != buffer[j])
			return buffer[i] < buffer[j] ? -1 : 1;

		// If both wrapped at least 3 times.
		if (i_wraps_left == 0 && j_wraps_left == 0)
			return 0; // They compare equal.
	}
}


/*
    Lexicographically compares two infinite cyclic sequences of strings,
    respectively:
     i ~ I: (buffer[i], buffer[i+1],..., buffer[k]), where next[k]<=k,
     j ~ J: (buffer[j], buffer[j+1],..., buffer[l]), where next[l]<=l

    Return values:
     If I < J, returns -1.
     If I > J, returns 1.
     If I = J and i < j, returns -1.
     If I = J and i >= j, returns 1.
*/
int bounded_compareS(unsigned int i, unsigned int j)
{
	// Save initial values to be able to wrap around.
	unsigned int iold = i;
	unsigned int jold = j;

	/*
	    Wrap counters to detect equality.
	*/
	auto i_wraps_left = uint8_t(3); // Formerly ic
	auto j_wraps_left = uint8_t(3); // Formerly jc

	if (buffer[i] != buffer[j])
		return buffer[i] < buffer[j] ? -1 : 1;

	for (;;)
	{
		// Scott:
		/* next find which has smallest jump */

		if (i < next[i]) // If the string I won't wrap around yet
		{
			if (j < next[j]) // If the string J won't wrap around yet
			{
				// If I has less repetitions of a single char than J
				if ((next[i] - i) < (next[j] - j))
				{
					// Compare next[i] with the repeating char in J.
					j += next[i] - i;
					i = next[i];
				}
				else if ((next[i] - i) > (next[j] - j)) // If I has more
				                                        // repetitions of a
				                                        // single char than J
				{
					// Compare next[j] with the repeating char in I.
					i += next[j] - j;
					j = next[j];
				}
				else // Either no repeating char or the same count of repeats.
				{
					i = next[i]; // Just compare next[i] with next[j]
					j = next[j];
				}
			}
			else // The string J wraps around now (Note: not necessarily to
			     // jold!)
			{
				if (j_wraps_left != 0) // J wrapped around.
					j_wraps_left--;

				j = jold; // Wrap it to jold.
				i++;      // Compare directly with the next in I (don't skip any
				          // repeats).
			}
		}
		else // The string I wraps around now (Note: not necessarily to iold!)
		{
			if (j < next[j]) // If the string J won't wrap around yet
			{
				j++; // Compare directly with the next in J (don't skip any
				     // repeats).
			}
			else // The string J wraps around as well (Note: not necessarily to
			     // jold!)
			{
				if (j_wraps_left != 0) // J wrapped around.
					j_wraps_left--;

				j = jold; // Wrap it to jold.
			}

			if (i_wraps_left != 0) // I wrapped around.
				i_wraps_left--;

			i = iold; // Wrap to iold.
		}

		// If I and J differ here.
		if (buffer[i] != buffer[j])
			return buffer[i] < buffer[j] ? -1 : 1;

		// If both wrapped at least 3 times.
		if (i_wraps_left == 0 && j_wraps_left == 0)
			return i < j ? -1 : 1;
	}
}


/*
    Partitions "buffer" into cycles stored in "next".
    Each cycle corresponds to a Lyndon word, or a Lyndon word power
    (i.e. a word repeated a number of times).

    Saves the last letter of each cycle in "last" at the index
    of the cycle's beginning.
*/
void part_cycle(unsigned int begin, unsigned int end)
{
	auto right = end; // Partition up till the end.

	// Form a single cycle in "next" that spans all chars in the buffer
	// and skips runs of same characters.
	Fill_bufs(begin, end);

	// Proceed to factorize the buffer.

	for (;;)
	{
		if (right == begin) // If just one letter left, it's a Lyndon word.
		{
			last[right] = buffer[right]; // Save the last letter of the cycle.
			next[right] = right;         // Only a single element in the cycle.

			break; // done
		}

		// Fix [begin,right] according to the Fill_bufs rules
		// Essentially next[right] = begin
		Mod_bufs(begin, right);

		/*
		    Find left, begin <= left <= right, such that the string
		    (buffer[left],...,buffer[right]) is a cycle (a Lyndon word or its
		    power).
		*/

		// Scott:
		//      left = right;                        // first guess
		//      for (unsigned int i = right; i-- > begin;) {
		//       if (bounded_compareS(left, i) != -1)
		//          left = i;
		//      }

		auto i = begin; // Start at begin: try for the longest possible cycle.
		auto left = right; // Scott: first guess

		while (i < right)
		{
			/*
			    I = (i,i+1,...,right) repeating
			    L = (left,left+1,...,right) repeating

			    if (I < L || (I == L && I <= L))
			*/
			if (bounded_compareS(left, i) != -1)
				left = i;

			i = next[i];
		}

		// (left,left+1,...,right) is the next cycle.

		// Fix the buffer - wrap the cycle
		// Essentially next[right] = left
		Mod_bufs(left, right);

		// Save the last character of the cycle.
		last[left] = buffer[right];

		if (left == begin) // If extracted the last cycle.
			break;

		// Proceed to find the next cycle.
		right = left - 1;

		// Redundant?
		last[begin] = buffer[right];
	}
}

int main(int argc, char* argv[])
{
	char chxx, chxy;
	int debug = 0;
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		debug = 1;
		argv++;
		argc--;
	}

	int blocksize = BLOCK_SIZE;
	if (argc > 1 && strcmp(argv[1], "-B") == 0)
	{
		blocksize = 100;
		argv++;
		argc--;
	}
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'b')
	{
		sscanf(argv[1], "%c%c%u", &chxx, &chxy, &blocksize);
		blocksize = (blocksize > 0 && blocksize < BLOCK_SIZE) ? blocksize : 256;
		fprintf(stderr, "New blocksize is %u \n", blocksize);
		argv++;
		argc--;
	}


	fprintf(stderr, "Performing BWTS version 20071215 on ");
	if (argc > 1)
	{
		freopen(argv[1], "rb", stdin);
		fprintf(stderr, "%s", argv[1]);
	}
	else
		fprintf(stderr, "stdin");
	fprintf(stderr, " to ");
	if (argc > 2)
	{
		freopen(argv[2], "wb", stdout);
		fprintf(stderr, "%s", argv[2]);
	}
	else
		fprintf(stderr, "stdout");

	fprintf(stderr, "\n");
#if !defined(unix)
	setmode(fileno(stdin), O_BINARY);
	setmode(fileno(stdout), O_BINARY);
#endif
	// Nelson:
	// This is the start of the giant outer loop.  Each pass
	// through the loop compresses up to BLOCK_SIZE characters.
	// When an fread() operation finally reads in 0 characters,
	// we break out of the loop and are done.
	//
	for (;;)
	{
		// Nelson:
		// After reading in the data into the buffer, I do some
		// UI stuff, then write the length out to the output
		// stream.
		//
		auto length = fread((char*)buffer, 1, blocksize, stdin);
		if (length == 0)
			break;

		if (length < 0 || length > blocksize)
		{
			fprintf(stderr, " Bad length file %lx \n", length);
			exit(0);
		}
		fprintf(stderr, "Performing BWTS on %ld bytes\n", length);

		// Insert all the indices to be sorted.
		int i;
		for (i = 0; i < length; i++)
			indices[i] = i;

		// Initialize "last" with a rotation of @c buffer by 1 to the right.
		last[0] = buffer[length - 1];
		for (i = 0; i < length - 1; i++)
			last[i + 1] = buffer[i];

		/*
		    Notice that now for all i except the currently unknown beginnings
		    of cycles, last[i] contains the correct last character of the
		    rotation of the cycle beginning at buffer[i].

		    See the description of "last".
		*/


		// Partition the buffer into cycles and save the last letter of each
		// cycle in last[cycle beginning index]
		part_cycle(0, length - 1);

		/*
		    part_cycle fixed "last" chars for cycle beginnings.

		    See the description of "last".
		*/


		// Sort all infinitely repeated rotations of the cycles.
		// Each index represents the cycle rotation beginning at that index.
		qsort(indices, length, sizeof(unsigned int),
		      (int (*)(const void*, const void*))bounded_compare);
		fprintf(stderr, "\n");

		// Print debug info.
		if (debug)
			dumpit(length);

		// Output last characters of the cycle rotations in sorted order.
		for (i = 0; i < length; i++)
		{
			fputc(last[indices[i]], stdout);
		}
	}
	return 0;
}