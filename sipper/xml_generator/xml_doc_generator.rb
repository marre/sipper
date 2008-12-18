require 'rexml/document'
require 'stringio'

module XmlDoc
  class RegInfoDoc
    include REXML
    
    #creates the registration information document (reginfo+xml) defined in RFC 3680
    #the current implementation supports only one 'registration' sub-element for an address-of-record.
    #the document contains the "full" registration state.
    #the contact elements supports the mandatory attributes: id, state and event
    # The arguments are the AOR, document version
    # and an array of Contact addresses. If contact address is nil, it picks the contacts from RegistrationStore 
    def self.create(aor, ver, contacts)
      doc = Document.new
      doc << XMLDecl.new
      reginfo = Element.new 'reginfo'
      reginfo.attributes["xmlns"] = 'urn:ietf:params:xml:ns:reginfo'
      reginfo.attributes["version"]= ver.to_s
      reginfo.attributes["state"]='full'
      
      regis = Element.new 'registration'
      reg_list = contacts ? contacts.to_a : SIP::Locator[:RegistrationStore].get(aor) 
      regis.attributes['aor']= aor
      regis.attributes['id']='a7'
      
      unless reg_list
        regis.attributes['state']='init'
      else
        regis.attributes['state']='active'
      
        reg_list.each do |data|
          contact = Element.new 'contact'
          contact.attributes['id'] = reg_list.index(data).to_s
          contact.attributes['state'] ='active'
          contact.attributes['event'] ='registered'
          uri = Element.new 'uri'
          uri.text = contacts ? data : data.contact_uri
          contact.add_element uri
          regis.add_element contact  
        end
      end  
      reginfo.add_element regis
      doc.add_element reginfo
      doc
    end
        
  end
end