
/db/cmd create table `player_account` ( `player_account_id` int auto_increment, `player_id` int default '0', `balance` int default '0', primary key (`player_account_id`))
/db/cmd create table `player_login` (`player_login_id` int auto_increment, `player_account_id` int, `login` timestamp, `logout` datetime default NULL, primary key(`player_login_id`))
/#/db/command create table `player_account` ( `player_account_id` int auto_increment, `player_id` int default '0', `balance` int default '0', primary key (`player_account_id`))


