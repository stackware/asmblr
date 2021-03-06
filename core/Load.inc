<?php
/**
 * @file Load.inc asmblr loader and global helper functions.
 * @author Stackware, LLC
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */


/**
 * asmblr root directory.
 */
define('ASM_ROOT',__DIR__.DIRECTORY_SEPARATOR);


/**
 * Shorthand for Log::Log().
 *
 * @see Log::Log()
 */
function llog( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
{
    \asm\Log::Log($Msg,$Level,$Backtrace,$Context);
}


/**
 * Shorthand for getting the $app object.
 *
 * @note Do not overuse.
 */
function asm()
{
    return $GLOBALS['asmapp'];
}


/**
 * Require the asmblr source code.
 */
require(ASM_ROOT.'Base.php');
require(ASM_ROOT.'Request.php');

require(ASM_ROOT.'App.php');

require(ASM_ROOT.'Page.php');
require(ASM_ROOT.'Template.php');
require(ASM_ROOT.'Link.php');

require(ASM_ROOT.'restr.php');
require(ASM_ROOT.'cnvyr.php');

require(ASM_ROOT.'Misc.php');

// Load the default en-US locale
require(ASM_ROOT.'enUS.php');

