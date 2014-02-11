// javascript helper with jquery for docbook toc toggle, namely the second section
// only works with dd, dl, dt output of docbook xml in html
// written by Michael Friedrich <michael.friedrich@gmail.com>
// (c) 2012-present Icinga Development Team


$(document).ready(function() {

	//by default, hide all sub sections (x.y.z)
	$('div.toc dl dd dl dd').hide();

	//only the second nested section, but not below with a [+]
	$("div.toc>dl>dd>dl>dt>span.section").each(function(index) {

		//the next element after 'dt' must be 'dd' to start a new subsection
		//so let's trigger that one, checking the length of the element>0 == exists
		if($(this).parent().next('dd').length > 0) {
			$(this).html($(this).html() + ' [+]');
		}
	});

	//bail early if the url of section x.y was clicked (no toggling here!)
	$("div.toc dl dd dl dt span.section a").click(function(e) {
		var target = $(e.target).attr('target');
		var href = $(e.target).attr('href');

		//open new window if ctrl key was pressed
		if(e.ctrlKey) {
			if(target === undefined) {
				target = '_blank';
			}
		}
		//open new window if middle mouse button was pressed
		if(e.which == 2) {
			if(target === undefined) {
				target = '_blank';
			}
		}

		//if target was undefined, open in the same window,
		//otherwise open a new window with the target identifier
		if(target !== undefined) {
			window.open(href, target);
		} else {
			window.location.href = href;
		}
		return false;
	});

	// the x.y will be the toggle element
	$("div.toc dl dd dl dt span.section").click(function(e) {

		//toggle everything from x.y.z til last-1 level
		//if taken.span.section, this will match til the end, which we don't want
		$(e.target).parent().nextUntil('div.toc dl dd dl dt').toggle();

		//FIXME find a way to replace the [+] on toggle with [-], and allow retoggle.
	        return false;
	});
});

