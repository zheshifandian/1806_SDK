/* The GPL applies to this program.
  In addition, as a special exception, the copyright holders give
  permission to link the code of portions of this program with the
  OpenSSL library under certain conditions as described in each
  individual source file, and distribute linked combinations
  including the two.
  You must obey the GNU General Public License in all respects
  for all of the code used other than OpenSSL.  If you modify
  file(s) with this exception, you may extend this exception to your
  version of the file(s), but you are not obligated to do so.  If you
  do not wish to do so, delete this exception statement from your
  version.  If you delete this exception statement from all source
  files in the program, then also delete it here.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "gen.h"

extern char last_error[];

ssize_t read_to(int fd, char *whereto, size_t len, int timeout)
{
	for(;;)
	{
		ssize_t rc;
		struct timeval to;
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		to.tv_sec  = timeout / 1000;
		to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

		rc = select(fd + 1, &rfds, NULL, NULL, &to);
		if (rc == 0)
			return RC_TIMEOUT;
		else if (rc == -1)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;

			snprintf(last_error, ERROR_BUFFER_SIZE, "myread::select failed: %s\n", strerror(errno));
			return RC_SHORTREAD;
		}

		return read(fd, whereto, len);
	}
}

ssize_t myread(int fd, char *whereto, size_t len, int timeout)
{
	ssize_t cnt=0;

	while(len>0)
	{
		ssize_t rc;
		struct timeval to;
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		to.tv_sec  = timeout / 1000;
		to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

		rc = select(fd + 1, &rfds, NULL, NULL, &to);
		if (rc == 0)
			return RC_TIMEOUT;
		else if (rc == -1)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;

			snprintf(last_error, ERROR_BUFFER_SIZE, "myread::select failed: %s\n", strerror(errno));
			return RC_SHORTREAD;
		}

		if (FD_ISSET(fd, &rfds))
		{
			rc = read(fd, whereto, len);

			if (rc == -1)
			{
				if (errno != EINTR && errno != EAGAIN)
				{
					snprintf(last_error, ERROR_BUFFER_SIZE, "myread::read failed: %s\n", strerror(errno));
					return RC_SHORTREAD;
				}
			}
			else if (rc == 0)
			{
				break;
			}
			else
			{
				whereto += rc;
				len -= rc;
				cnt += rc;
			}
		}
	}

	return cnt;
}

ssize_t mywrite(int fd, char *wherefrom, size_t len, int timeout)
{
	ssize_t cnt=0;

	while(len>0)
	{
		ssize_t rc;
		struct timeval to;
		fd_set wfds;

		FD_ZERO(&wfds);
		FD_SET(fd, &wfds);

		to.tv_sec  = timeout / 1000;
		to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

		rc = select(fd + 1, NULL, &wfds, NULL, &to);
		if (rc == 0)
			return -2;
		else if (rc == -1)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;

			snprintf(last_error, ERROR_BUFFER_SIZE, "mywrite::select failed: %s\n", strerror(errno));
			return -1;
		}

		rc = write(fd, wherefrom, len);

		if (rc == -1)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				snprintf(last_error, ERROR_BUFFER_SIZE, "mywrite::write failed: %s\n", strerror(errno));
				return -1;
			}
		}
		else if (rc == 0)
		{
			break;
		}
		else
		{
			wherefrom += rc;
			len -= rc;
			cnt += rc;
		}
	}

	return cnt;
}

int set_fd_nonblocking(int fd)
{
        /* set fd to non-blocking */
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, "set_fd_nonblocking failed! (%s)\n", strerror(errno));

                return -1;
	}

        return 0;
}

int set_fd_blocking(int fd)
{
        /* set fd to blocking */
        if (fcntl(fd, F_SETFL, 0) == -1)
	{
		fprintf(stderr, "set_fd_blocking failed! (%s)\n", strerror(errno));

                return -1;
	}

        return 0;
}
