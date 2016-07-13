/*-
 * Copyright 2016 Massachusetts Institute of Technology
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifdef __linux__
#define _GNU_SOURCE 1
#endif

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

int
main(int argc, char *const *argv)
{
	int opt, multiple_files, quiet, rc;

	quiet = 0;
	while ((opt = getopt(argc, argv, "q")) != EOF) {
		switch (opt) {
		case 'q':
			quiet++;
			break;
		default:
			exit(EX_USAGE);
		}
	}
	argc -= optind;
	argv += optind;

	if (*argv == NULL) {
		errx(EX_USAGE, "must supply at least one file to scan");
	}

	rc = EXIT_SUCCESS;
	multiple_files = argv[1] != NULL;

	for (; *argv != NULL; argv++) {
		const char *fn = *argv;
		off_t current;
		int fd, count;
		long min_size;
		intmax_t size, total_size;

		fd = open(fn, O_RDONLY, 0);
		if (fd == -1) {
			err(1, "%s", fn);
		}
		errno = 0;
#ifndef __linux__
		/* Apparently there is no way on Linux to find out if this
		   even works at all. */
		min_size = fpathconf(fd, _PC_MIN_HOLE_SIZE);
		if (errno != 0) {
			warn("%s: fpathconf(_PC_MIN_HOLE_SIZE)", fn);
			rc = EXIT_FAILURE;
			goto next_file;
		} else if (min_size <= 0) {
			warnx("%s: filesystem does not support holes", fn);
			rc = EXIT_FAILURE;
			goto next_file;
		}
#endif

		current = 0;
		count = 0;
		total_size = 0;
		do {
			off_t hole_start;
	
			if ((hole_start = lseek(fd, current, SEEK_HOLE)) == -1) {
				if (errno == ENXIO)
					break;
				warn("%s: SEEK_HOLE", fn);
				rc = EXIT_FAILURE;
				goto next_file;
			}
			if ((current = lseek(fd, hole_start, SEEK_DATA)) == -1) {
				if (errno == ENXIO)
					break;

				warn("%s: SEEK_DATA", fn);
				rc = EXIT_FAILURE;
				goto next_file;
			}

			size = current - hole_start;
			total_size += size;
			if (!quiet) {
				if (count == 0 && multiple_files)
					printf("%s:\n", fn);

				printf("%jd-%jd (%jd)\n", (intmax_t)hole_start, 
				    (intmax_t)current, size);
			}
			count++;
		} while (1);
	
		if (count != 0 || !quiet) {
			if (multiple_files)
				printf("%s: ", fn);
			printf("%d holes, %jd bytes\n", count, total_size);
		}
next_file:
		close(fd);
	}
	return rc;
}
