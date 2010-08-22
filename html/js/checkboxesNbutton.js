<!-- Written by Rune Darrud -->
<!--     For Icinga         -->
function replaceCGIString(string,status_cgi,cmd_cgi)
{ 
	sInString = string.replace( status_cgi, cmd_cgi ); 
	return sInString;
}
function replaceArgString(string)
{ 
	ToBeStripped = location.search; 
	sInString = string.replace( ToBeStripped, '' ); 
	return sInString;
}
function cmd_submit(form)
{
	command_arguments = get_check_value(form);
	cmd_typ = 'cmd_typ=' + document.tableform.hiddencmdfield.value
	if (document.tableform.hiddenforcefield.value == 'yes')
	{
			arguments = cmd_typ + command_arguments + '&force_check';
	}
	else
	{
			arguments = cmd_typ + command_arguments;
	}
	bazinga = '?' + arguments;
	fullurl = replaceCGIString(location.href,'status.cgi','cmd.cgi');
	fullurl = replaceArgString(fullurl);
        fullurl = fullurl + bazinga;
        self.location.assign(fullurl);
	// Remove comment below for debugging of the URL
	//alert(fullurl);
        return fullurl;
}
function isValidForSubmit(form)
{
        var group = document.getElementById(form);
        var x, len = group.length;
        for(x=0; x<len; x++)
        {
                if(group[x].checked)
                {
                        break;
                }
        }
        if(x < len)
        {
                if (document.tableform.serviceTotalsCommandsButton)
                        {
                                document.tableform.buttonCheckboxChecked.value='true';
                        }
                if (document.tableform.hostTotalsCommandsButton)
                        {
                                document.tableform.buttonCheckboxChecked.value='true';
                        }
                enableDisableButton();
        }
        else
        {
                if (document.tableform.serviceTotalsCommandsButton)
                        {
                                document.tableform.buttonCheckboxChecked.value='false';
                        }
                if (document.tableform.hostTotalsCommandsButton)
                        {
                                document.tableform.buttonCheckboxChecked.value='false';
                        }
                enableDisableButton();
                return false;
        }
        return true;
}
function enableDisableButton()
{
        if (document.tableform.buttonValidChoice.value=='true'){
                if (document.tableform.buttonCheckboxChecked.value=='true'){
                        if (document.tableform.serviceTotalsCommandsButton){
                                document.tableform.serviceTotalsCommandsButton.disabled=false;
                        }
                        if (document.tableform.hostTotalsCommandsButton){
                                document.tableform.hostTotalsCommandsButton.disabled=false;
                        }
                } else {
                        if (document.tableform.serviceTotalsCommandsButton){
                                document.tableform.serviceTotalsCommandsButton.disabled=true;
                        }
                        if (document.tableform.hostTotalsCommandsButton){
                                document.tableform.hostTotalsCommandsButton.disabled=true;
                        }
                }
	}
        if (document.tableform.buttonCheckboxChecked.value=='true'){
                if (document.tableform.buttonValidChoice.value=='true'){
                        if (document.tableform.serviceTotalsCommandsButton){
                                document.tableform.serviceTotalsCommandsButton.disabled=false;
                        }
                        if (document.tableform.hostTotalsCommandsButton){
                                document.tableform.hostTotalsCommandsButton.disabled=false;
                        }
                } else {
                        if (document.tableform.serviceTotalsCommandsButton){
                                document.tableform.serviceTotalsCommandsButton.disabled=true;
                        }
                        if (document.tableform.hostTotalsCommandsButton){
                                document.tableform.hostTotalsCommandsButton.disabled=true;
                        }
                }
        }
}
