<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:svg="http://www.w3.org/2000/svg">
<xsl:template match="/">
<svg xmlns="http://www.w3.org/2000/svg" height="128" width="128" version="1.0">
<defs>
    <xsl:copy-of select="document('filters.svg')/svg:svg/svg:defs/svg:filter"/>
    <xsl:copy-of select="document('background.svg')/svg:svg/svg:defs"/>
    <xsl:copy-of select="svg:svg/svg:defs"/>
</defs>
<g filter="url(#{$filter_name})">
    <xsl:copy-of select="document('background.svg')/svg:svg/svg:g"/>
</g>
<g transform="scale(0.80) translate(15, 20)">
    <xsl:copy-of select="svg:svg/svg:g"/>
</g>
</svg>
</xsl:template>
</xsl:stylesheet>

