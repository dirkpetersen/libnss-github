# libnss_github

`libnss-github` is a database based backend for [GNU Libc Name Service Switch.](https://www.gnu.org/software/libc/manual/html_node/Name-Service-Switch.html)  It can get users and groups from a remote database. (currently implemented using the [Redis](https://redis.io/) protocol)

This backend has been created for use cases where Linux users need to collaborate across multiple different organizations who decided to use a coordinated approach for UID/GID (for example High performance computing systems). As there are about 100M Github users at this time and Github has public alphanumeric 'login' and numeric 'id' (for example https://api.github.com/users/jimmy) we can have our users create github accounts and assign the numberic id of their github account to their enterprise identity (out of scope of this project) 

## Using redis as a database 

Since 2020 there have been around 17 million github users with public activity which results in about 70 millon database entries (one each for user,uid,group,gid). 
We brought up a Redis compatible server using the [kvrocks](https://kvrocks.apache.org/) implementation of the Redis protocol and used the smallest possible Linode host (Nanode, 1GB RAM, 1 CPU, 25GB Flash, $5/month) in their Dallas datacenter (~ 50 ms latency from the US Pacific Northwest as well as the Northeast). The database is about 12GB and the overall disk consumption of the system is about 85% while 60% of the memory is used by kvrocks.

### install 

we are using Ubuntu 22.04 for this test and install the low level hiredis library and copy the binary `libnss-github` library to the right location. We compiled the credentials and hostname for the linode server compiled into the libnss library.  

```
sudo apt install -y libhiredis0.14
sudo cp -f bin/ubuntu-22.04/libnss_github.so.2 /usr/lib/
sudo chmod 755 /usr/lib/libnss_github.so.2
```

### configure

All we need to do is to add `github` entries to the passwd and group services in nsswitch.conf, we insert these entries right behind the `systemd` entries. You might also find `compat`, ldap or `sssd` entries. Make sure that `github` is the last entry at the end of each line. You can execute this command to take care of the change on stock Ubuntu: 

```
sudo sed -i -e "s/systemd/systemd github/g" /etc/nsswitch.conf
```

### Test

First let's see how fast we can retrieve user info from the github database with 70M overall records. we query this directly on the Redis/kvrocks server to avoid network latency 

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

The final test is from a system that is hooked up through a t-mobile hotspot and has 100-300ms latency

```
dp@wsl:~$ time getent passwd jimsmith
jimsmith:x:2676701:2676701:jimsmith:/:/sbin/nologin

real    0m0.373s
user    0m0.007s
sys     0m0.002s
```

We could dramatically reduce this latency with a tiered / cached architecture which would be very inexpensive 

## Classic compile / install 

### build libnss-github

edit config/header file `config.h`, install hiredis dependency, compile and install:

```
sudo dnf install -y libhiredis-devel || \
  sudo apt install -y libhiredis-dev
sudo rm /usr/lib/libnss_github.so.2; \
  make clean && make && \
  sudo make install
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
passwd:         compat github
group:          compat github
shadow:         compat
```

##Missing features

 * Listing users : `getent passwd|group`
 * shadow support
 * write support 

##Security considerations

Always use as last entry (after compat,systemd,sssd) in nsswitch.conf,
otherwise it could overwrite the shadow-password for root.
(shadow has no uids, so this cannot be ruled out)

	If someone is able to place terminals instead of the
	files, that could cause all programs to get a new
	controling terminal, making DoS attacks possible.


## Author

Frédéric VANNIÈRE <f.vanniere@planet-work.com>
