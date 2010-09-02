/* JavaScript for (un)checking all checkboxes */
<!--        Script by hscripts.com          -->
<!--        copyright of HIOX INDIA         -->
<!-- Free javascripts @ http://www.hscripts.com -->

checked=false;
function checkAll (form) {
		var checkboxes= document.getElementById(form);
		if (checked == false)
		{
				checked = true
		}
		else
		{
				checked = false
		}
		for (var i =0; i < checkboxes.elements.length; i++)
		{
				checkboxes.elements[i].checked = checked;
		}
}

<!-- Script by hscripts.com -->
