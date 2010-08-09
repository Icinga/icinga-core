function showValue(arg,schedule_host_check,schedule_host_svc_check) {
	if (arg!='nothing'){
		document.tableform.hiddencmdfield.value = arg;
		// Set the value to true.
        	document.tableform.buttonValidChoice.value = 'true';

		if (arg==schedule_host_check || arg==schedule_host_svc_check) {
                	document.tableform.hiddenforcefield.value = 'yes';
		} else {
                	document.tableform.hiddenforcefield.value = 'no';
        	}
		enableDisableButton();
	} else {
		// Set the value to false, cant submit
	        document.tableform.buttonValidChoice.value = 'false';
		enableDisableButton();
	}
}
