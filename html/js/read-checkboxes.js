function get_check_value()
{
var check_value = '';
var checkboxes= document.getElementById('tableform');
checked = true
for (var i =0; i < checkboxes.elements.length; i++)
{
	if (checkboxes.elements[i].checked==checked){
		check_value = check_value + checkboxes.elements[i].value;}
}
//Remove comment below for debugging
//alert(check_value);
return check_value;
}
