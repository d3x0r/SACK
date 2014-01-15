<?php 
/***************************** 
  File: login.php 
  Written by: Frost of Slunked.com 
  Tutorial: User Registration and Login System 
******************************/ 

require($_SERVER['DOCUMENT_ROOT'] . '/login/includes/config.php'); 
 
// If the user is logging in or out 
// then lets execute the proper functions 
if (isset($_GET['action'])) { 
  switch (strtolower($_GET['action'])) { 
    case 'login': 
      if (isset($_POST['username']) && isset($_POST['password'])) { 
        // We have both variables. Pass them to our validation function 
        if (!validateUser($_POST['username'], $_POST['password'])) { 
          // Well there was an error. Set the message and unset 
          // the action so the normal form appears. 
          $_SESSION['error'] = "Bad username or password supplied."; 
          unset($_GET['action']); 
        } 
      }else { 
        $_SESSION['error'] = "Username and Password are required to login."; 
        unset($_GET['action']); 
      }       
    break; 
    case 'logout': 
      // If they are logged in log them out. 
      // If they are not logged in, well nothing needs to be done. 
      if (loggedIn()) { 
        logoutUser(); 
        $sOutput .= '<h1>Logged out!</h1><br />You have been logged out successfully.  
            <br /><h4>Would you like to go to <a href="index.php">site index</a>?</h4>'; 
      }else { 
        // unset the action to display the login form. 
        unset($_GET['action']); 
      } 
    break; 
  } 
} 
 
$sOutput .= '<div id="index-body">'; 
 
// See if the user is logged in. If they are greet them  
// and provide them with a means to logout. 
if (loggedIn()) { 
  $sOutput .= '<h1>Logged In!</h1><br /><br /> 
    Hello, ' . $_SESSION["username"] . ' how are you today?<br /><br /> 
    <h4>Would you like to <a href="login.php?action=logout">logout</a>?</h4> 
    <h4>Would you like to go to <a href="index.php">site index</a>?</h4>'; 
}elseif (!isset($_GET['action'])) { 
  // incase there was an error  
  // see if we have a previous username 
  $sUsername = ""; 
  if (isset($_POST['username'])) { 
    $sUsername = $_POST['username']; 
  } 
   
  $sError = ""; 
  if (isset($_SESSION['error'])) { 
    $sError = '<span id="error">' . $_SESSION['error'] . '</span><br />'; 
  } 
   
  $sOutput .= '<h2>Login to our site</h2><br /> 
    <div id="login-form"> 
      ' . $sError . ' 
      <form name="login" method="post" action="login.php?action=login"> 
        Username: <input type="text" name="username" value="' . $sUsername . '" /><br /> 
        Password: <input type="password" name="password" value="" /><br /><br /> 
        <input type="submit" name="submit" value="Login!" /> 
      </form> 
    </div> 
    <h4>Would you like to <a href="login.php">login</a>?</h4> 
    <h4>Create a new <a href="register.php">account</a>?</h4>'; 
} 
 
$sOutput .= '</div>'; 
 
// lets display our output string. 
echo $sOutput; 
?>