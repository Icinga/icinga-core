// javascript helper with jquery for docbook toc toggle, namely the second section
// written by Michael Friedrich
// (c) 2012 Icinga Development Team


$(document).ready(function() {
    $('div.toc dl dd dl dd').hide();

    $("div.toc dl dd dl dt span.section").click(function(e) {
        $(e.target).parent().nextUntil('div.toc dl dd dl dt span.section').toggle();
        return false;
    });
});

