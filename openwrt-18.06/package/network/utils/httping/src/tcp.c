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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gen.h"
#include "io.h"
#include "tcp.h"

extern char last_error[];

int connect_to(struct sockaddr *bind_to, struct addrinfo *ai, int timeout)
{
	int     fd;
	int 	rc;
	struct timeval to;
	fd_set wfds;

	/* create socket */
	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (fd == -1)
	{
		snprintf(last_error, ERROR_BUFFER_SIZE, "problem creating socket (%s)", strerror(errno));
		return -1;
	}

	/* go through a specific interface? */
	if (bind_to)
	{
		int set = 1;

		/* set reuse flags */
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) == -1)
		{
			snprintf(last_error, ERROR_BUFFER_SIZE, "error setting sockopt to interface (%s)", strerror(errno));
			return -1;
		}

		if (bind(fd, bind_to, sizeof(*bind_to)) == -1)
		{
			snprintf(last_error, ERROR_BUFFER_SIZE, "error binding to interface (%s)", strerror(errno));
			return -1;
		}
	}

	/* make fd nonblocking */
	if (set_fd_nonblocking(fd) == -1)
		return -1;

	/* wait for connection */
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	to.tv_sec  = timeout / 1000;
	to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

	/* connect to peer */
	if (connect(fd, ai -> ai_addr, ai -> ai_addrlen) == 0)
	{
		/* connection made, return */
		return fd;
	}

	/* wait for connection */
	rc = select(fd + 1, NULL, &wfds, NULL, &to);
	if (rc == 0)
	{
		close (fd);
		return -2;	/* timeout */
	}
	else if (rc == -1)
	{
		close(fd);
		if (errno == EINTR)
			return -3;	/* ^C pressed */
		else
			return -1;	/* error */
	}
	else
	{
		int optval=0;
		socklen_t optvallen=sizeof(optval);

		/* see if the connect succeeded or failed */
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optvallen) == -1)
		{
			snprintf(last_error, ERROR_BUFFER_SIZE, "getsockopt failed (%s)\n", strerror(errno));
			return -1;
		}

		/* no error? */
		if (optval == 0)
			return fd;

		/* don't ask */
		errno = optval;
	}

	close(fd);

	snprintf(last_error, ERROR_BUFFER_SIZE, "could not connect (%s)\n", strerror(errno));

	return -1;
}

int set_tcp_low_latency(int sock)
{
	int flag = 1;

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
	{
		snprintf(last_error, ERROR_BUFFER_SIZE, "could not set TCP_NODELAY on socket (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}
