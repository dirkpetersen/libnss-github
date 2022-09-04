# libnss_redis

`libnss-redis` is a [Redis](https://redis.io/) dabatase backend for[GNU Libc Name Service Switch.](https://www.gnu.org/software/libc/manual/html_node/Name-Service-Switch.html)  It can get users and groups from a remote database.

This backend has been created for mass web hosting with dynamic configuration.

## Testing with Github users.

One way to show the performance of Redis is to use a large database. We brought up a Redis server using the kvrocks implementation of the redis protocol on the smallest possible Linode host (Nanode, 1GB RAM, 1 CPU, 25GB Flash, $5/month) in their Dallas datacenter (~ 50 ms latency from the pacific northwest west as well as the north east). The server hosts the usernames and numerical ids of about 17 million github users that have had public activity since 2020. The database is about 12GB and the overall disk consumption of the system is about 85% while 60% of the memory is used for the kvrocks database. We would like to use this database for the Name Service Switch

### install 

we are using Ubuntu 22.04 for this test and install the low level hiredis library and copy the binary libnss-redis library to the right location. We compiled the credentials and hostname for the linode server compiled into the libnss library.  

```
sudo apt install -y libhiredis0.14
sudo cp bin/ubuntu-22.04/libnss_redis.so.2 /usr/local/lib/
sudo chmod 755 /usr/local/lib/libnss_redis.so.2
```

### configure

all we need to do is to add redis entries to the passwd and group services in nsswitch.conf, we insert these entries right behind the systemd entries. You might also find `compat`, ldap or `sssd` entries. Make sure that redis is the last entry. 

```
sudo sed -i -e "s/systemd/systemd redis/g" /etc/nsswitch.conf
```

### Test

First let's see how fast we can retrieve user info from the github database with 70M overall records. we query this directly on the redis/kvrocks server to avoid network latency 

```
root@kvrocks:~# time getent passwd king
king:x:13588878:13588878:king:/:/sbin/nologin

real    0m0.003s
user    0m0.002s
sys     0m0.000s
```

0.003s is not bad for quering a non-cached record stored on SSD of such a large system. Now let's query this database from a server in Portland, OR with about 50 ms latency. The current implementation requires at least 2 requests so overall we have more than 100 ms delay, still not bad 

```
ubuntu@kvrockstest:~$ time getent passwd johncarpenter
johncarpenter:x:739461:739461:johncarpenter:/:/sbin/nologin

real    0m0.163s
user    0m0.003s
sys     0m0.000s
```

the final test is from a system that is hooked up through a t-mobile hotspot and has 100-300ms latency


```
dp@wsl:~$ time getent passwd jimsmith
jimsmith:x:2676701:2676701:jimsmith:/:/sbin/nologin

real    0m0.373s
user    0m0.007s
sys     0m0.002s
```

We could dramatically reduce this latency with a tiered / cached architecture which would be very inexpensive 

## Classic compile / install 

### build libnss-redis

edit config/header file `config.h`, install hiredis dependency, compile and install:

```
sudo dnf install -y libhiredis-devel || \
  sudo apt install -y libhiredis-dev
make clean && make && \
  sudo rm /usr/lib/libnss_redis.so.2 && \
  sudo cp libnss_redis.so.2 /usr/lib/ && \
  sudo chmod 755 /usr/lib/libnss_redis.so.2
```

### Redis database

The connection configuration is done at compile time with values in `config.h`. The socket will be use if `REDIS_SOCKET` is defined.

The database format is :

* keys: USER/_username_, USER/_uid_, GROUP/_groupname_, GROUP/_gid_
* Values: same format as in `/etc/passwd` and `/etc/group`

There are examples in `.drone.yml` configuration file.

### Local configuration

With the following lines in `/etc/nsswitch.conf`

```
passwd:         compat redis
group:          compat redis
shadow:         compat
```

##Missing features

 * Listing users : `getent passwd|group`
 * shadow support
 * write support 

##Security considerations

Always use after compat in nsswitch.conf,
otherwise it could overwrite the shadow-password for root.
(shadow has no uids, so this cannot be ruled out)

	If someone is able to place terminals instead of the
	files, that could cause all programs to get a new
	controling terminal, making DoS attacks possible.


## Author

Frédéric VANNIÈRE <f.vanniere@planet-work.com>
