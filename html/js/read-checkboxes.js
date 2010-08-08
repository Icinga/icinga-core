function get_check_value()
{
var check_value = '';
var url_value = '';
var array = '';
for (var i=0; i < document.tableform.checkbox.length; i++)
   {
   if (document.tableform.checkbox[i].checked) 
		  {
		  check_value = check_value + document.tableform.checkbox[i].value;
		  }
   }
return check_value;
}
