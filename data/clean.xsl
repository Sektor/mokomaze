<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:svg="http://www.w3.org/2000/svg">
<xsl:output omit-xml-declaration="yes"/>
<xsl:template match="node()|@*">
    <xsl:copy>
        <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
</xsl:template>
<xsl:template match="svg:g[@style='display:none']"/>
<xsl:template match="svg:path[@style='display:none']"/>
</xsl:stylesheet>

