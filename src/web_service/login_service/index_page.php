<?php 
/***************************** 
  File: index.php 
  Written by: Frost of Slunked.com 
  Tutorial: User Registration and Login System 
******************************/ 
require($_SERVER['DOCUMENT_ROOT'] . 'login/includes/config.php'); 
 
$sOutput .= '<div id="index-body">'; 
if (loggedIn()) { 
  $sOutput .= '<h2>Welcome!</h2> 
    Hello, ' . $_SESSION['username'] . ' how are you today?<br /> 
    <h4>Would you like to <a href="login.php?action=logout">Logout?</a></h4>'; 
}else { 
  $sOutput .= '<h2>Welcome to our site</h2><br /> 
    <h4>Would you like to <a href="login.php">login</a>?</h4> 
    <h4>Create a new <a href="register.php">account</a>?</h4>'; 
 
} 
$sOutput .= '</div>'; 
 
echo $sOutput; 
?>
