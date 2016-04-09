### original ###
http://muddledramblings.com/rumblings-from-the-secret-labs/lamp-server-from-scratch-with-macports/

### prepare ###
```
sudo port -vd selfupdate
# sudo port -vd upgrade outdated
```

### apache2 ###
```
sudo port -vd install apache2
# sudo port -vd load apache2

# vi ~/.bash_profile
# alias apache2ctl='sudo /opt/local/apache2/bin/apachectl'
# source ~/.base_profile

# apache2ctl start
```
http://127.0.0.1

### php54 ###
```
sudo port -vd install php54
sudo port -vd install php54-apache2handler

sudo cp /opt/local/etc/php54/php.ini-development /opt/local/etc/php54/php.ini
php -i

# cd /opt/local/apache2/conf
# sudo cp httpd.conf httpd.conf.backup

# cd /opt/local/apache2/modules
# sudo /opt/local/apache2/bin/apxs -a -e -n "php5" libphp54.so

# vi /opt/local/apache2/conf/httpd.conf
# ...
# AddType text/html .php
# ...
# AddHandler application/x-httpd-php .php
# AddHandler application/x-httpd-php-source .phps
# ...
# DirectoryIndex index.html index.php
# ...
# Options FollowSymLinks

# /opt/local/apache2/bin/httpd -t
# apache2ctl restart
```

### index.php ###
```
# cd /opt/local/apache2/htdocs
# vi index.php
<html>
  <body>
    <h1>It works!</h1>
    <?php echo phpinfo(); ?>
  </body>
</html>
```
http://127.0.0.1/index.php

### mysql5 ###
```
sudo port -vd install mysql5-server
sudo port -vd load mysql5-server

sudo -u _mysql mysql_install_db5

# vi ~/.base_profile
# alias mysqlstart='sudo /opt/local/share/mysql5/mysql/mysql.server start'
# alias mysql='/opt/local/lib/mysql5/bin/mysql'
# alias mysqladmin='/opt/local/lib/mysql5/bin/mysqladmin'
# source ~/.base_profile

# mysqlstart
# sudo /opt/local/lib/mysql5/bin/mysql_secure_installation

# mysqladmin -u root -p ********
# sudo cp /opt/local/share/mysql5/mysql/my-medium.cnf <basedir>/my.cnf
```

### php54-mysql ###
```
# vi /opt/local/etc/php54/php.ini
# pdo_mysql.default_socket = <paste here>
# ...
# mysql.default_socket = <paste here>
# ...
# mysqli.default_socket = <paste here>
```
[set a default time zone.](http://php.net/manual/en/datetime.configuration.php)

```
sudo port -vd install php54-mysql

apache2ctl restart

# php -i | grep -i 'mysql'
```

### testmysql.php ###
```
    <?php
    $dbhost = 'localhost';
    $dbuser = 'root';
    $dbpass = 'MYSQL_ROOT_PASSWRD';
    $conn = mysql_connect($dbhost, $dbuser, $dbpass);
    if ($conn) {
        echo 'CONNECT OK';
    } else {
        die ('Error connecting to mysql');
    }
    $dbname = 'mysql';
    mysql_select_db($dbname);
```
http://127.0.0.1/testmysql.php
```
sudo rm /opt/local/apache2/htdocs/testmysql.php
```