<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- 닥북 한글 스타일시트 (http://kldp.net/projects/docbook/) -->
<!-- $Id: dbk-htmlhelp.xsl,v 1.1 2003/05/18 01:16:02 minskim Exp $ -->
	   
<xsl:import href="dbk-html-chunk.xsl"/>
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/htmlhelp/htmlhelp-common.xsl"/>

<xsl:param name="htmlhelp.encoding" select="'euc-kr'"/>

</xsl:stylesheet>
